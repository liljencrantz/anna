//ROOT: src/lib/lang/lang.c

#define MUTABLE_OFF 0
#define IMUTABLE_OFF 1
#define ANY_OFF 2

anna_type_t *mutable_list_type = 0;
anna_type_t *imutable_list_type = 0;
anna_type_t *any_list_type = 0;
static hash_table_t anna_list_specialization;
static array_list_t anna_list_pushitional_methods[] = {AL_STATIC, AL_STATIC, AL_STATIC};

static anna_function_t *anna_list_specialize(
    anna_type_t *type, anna_function_t *fun)
{
    anna_node_call_t *spec = anna_node_create_call2(
	0, anna_node_create_null(0), 
	anna_node_create_dummy(
	    0, 
	    anna_type_wrap(
		(anna_type_t *)anna_entry_get_static_obj(
		    type,
		    ANNA_MID_LIST_SPECIALIZATION))));
    
    anna_function_t *res = anna_function_compile_specialization(
	fun, spec);
    
    return res;
}

static void anna_list_push_method_internal(
    anna_type_t *type, anna_function_t *fun)
{
    anna_function_t *fun_spec = anna_list_specialize(type, fun);
    if(fun_spec)
    {
	anna_member_create_method(type, anna_mid_get(fun->name), fun_spec);
    }
}

static void add_any_list_method(void *key, void *value, void *aux)
{
    anna_type_t **list = (anna_type_t **)value;
    anna_function_t *fun = (anna_function_t *)aux;
//    anna_message(L"Add function %ls to type %ls\n", fun->name, list[ANY_OFF]->name);
    anna_list_push_method_internal(list[ANY_OFF], fun);
}

static void add_mutable_list_method(void *key, void *value, void *aux)
{
    anna_type_t **list = (anna_type_t **)value;
    anna_function_t *fun = (anna_function_t *)aux;
//    anna_message(L"Add function %ls to type %ls\n", fun->name, list->name);
    anna_list_push_method_internal(list[MUTABLE_OFF], fun);
}

static void add_imutable_list_method(void *key, void *value, void *aux)
{
    anna_type_t **list = (anna_type_t **)value;
    anna_function_t *fun = (anna_function_t *)aux;
//    anna_message(L"Add function %ls to type %ls\n", fun->name, list->name);
    anna_list_push_method_internal(list[IMUTABLE_OFF], fun);
}

void anna_list_any_add_method(anna_function_t *fun)
{
//    anna_message(L"Function %ls to all list types\n", fun->name);
    al_push(&anna_list_pushitional_methods[ANY_OFF], fun);
    hash_foreach2(&anna_list_specialization, &add_any_list_method, fun);
}

void anna_list_mutable_add_method(anna_function_t *fun)
{

//    anna_message(L"Function %ls to all list types\n", fun->name);
    al_push(&anna_list_pushitional_methods[MUTABLE_OFF], fun);
    hash_foreach2(&anna_list_specialization, &add_mutable_list_method, fun);
}

void anna_list_imutable_add_method(anna_function_t *fun)
{
//    anna_message(L"Function %ls to all list types\n", fun->name);
    al_push(&anna_list_pushitional_methods[IMUTABLE_OFF], fun);
    hash_foreach2(&anna_list_specialization, &add_imutable_list_method, fun);
}

static void anna_list_push_all_extra_methods(anna_type_t *list, int list_type)
{
    int i;
    for(i=0; i<al_get_count(&anna_list_pushitional_methods[list_type]); i++)
    {
	anna_function_t *fun = (anna_function_t *)al_get(&anna_list_pushitional_methods[list_type], i);
//	anna_message(L"Add function %ls to type %ls\n", fun->name, list->name);
//	anna_member_create_method(list, anna_mid_get(fun->name), fun);
	anna_list_push_method_internal(list, fun);
    }
}

anna_object_t *anna_list_create_imutable(anna_type_t *spec)
{
    return anna_object_create(anna_list_type_get_imutable(spec));
}

anna_object_t *anna_list_create_mutable(anna_type_t *spec)
{
    return anna_object_create(anna_list_type_get_mutable(spec));
}

anna_object_t *anna_list_create2(anna_type_t *list_type)
{
    return anna_object_create(list_type);
}

static void anna_list_type_init(anna_object_t *obj)
{
    anna_entry_set_obj(obj,ANNA_MID_LIST_PAYLOAD, 0);
    anna_entry_set_obj(obj,ANNA_MID_LIST_CAPACITY, 0);    
    anna_entry_set_obj(obj,ANNA_MID_LIST_SIZE, 0);
    obj->flags |= ANNA_OBJECT_LIST;
}

static anna_type_t *anna_list_get_specialization(anna_object_t *obj)
{
    return (anna_type_t *)
	anna_entry_get_obj(
	    obj,
	    ANNA_MID_LIST_SPECIALIZATION);
}

static int anna_list_is_mutable(anna_object_t *obj)
{
    return obj->type->name[0] == L'M';
}

void anna_list_set(struct anna_object *this, ssize_t offset, anna_entry_t value)
{
    size_t size = anna_list_get_count(this);
    ssize_t pos = anna_idx_wrap(offset, size);
    //anna_message(L"Set el %d in list of %d elements\n", pos, size);
    if(pos < 0)
    {
	return;
    }
    if(pos >= size)
    {
	anna_list_set_count(this, pos+1);      
    }
    
    anna_entry_t *ptr = anna_list_get_payload(this);
    ptr[pos] = value;  
}

anna_entry_t anna_list_get(anna_object_t *this, ssize_t offset)
{
    size_t size = anna_list_get_count(this);
    ssize_t pos = anna_idx_wrap(offset, size);
    anna_entry_t *ptr = anna_list_get_payload(this);
    if(pos < 0||pos >=size)
    {
	return null_entry;
    }
    return ptr[pos];
}

void anna_list_push(struct anna_object *this, anna_entry_t value)
{
    size_t size = anna_list_get_count(this);
    anna_list_set(this, size, value);
}

size_t anna_list_get_count(anna_object_t *this)
{
    return *(size_t *)anna_entry_get_addr(this,ANNA_MID_LIST_SIZE);
}

void anna_list_set_count(anna_object_t *this, size_t sz)
{
    size_t old_size = anna_list_get_count(this);
    size_t capacity = anna_list_get_capacity(this);
    
    if(sz>old_size)
    {
	if(sz>capacity)
	{
	    anna_list_set_capacity(this, sz);
	}
	anna_entry_t *ptr = anna_list_get_payload(this);
	int i;
	for(i=old_size; i<sz; i++)
	{
	    ptr[i] = null_entry;
	}
    }
    *(size_t *)anna_entry_get_addr(this,ANNA_MID_LIST_SIZE) = sz;
}

size_t anna_list_get_capacity(anna_object_t *this)
{
    return *(size_t *)anna_entry_get_addr(this,ANNA_MID_LIST_CAPACITY);
}

void anna_list_set_capacity(anna_object_t *this, size_t sz)
{
    anna_entry_t *ptr = anna_list_get_payload(this);
    ptr = realloc(ptr, sizeof(anna_entry_t )*sz);
    if(!ptr)
    {
	CRASH;
    }    
    (*(size_t *)anna_entry_get_addr(this,ANNA_MID_LIST_CAPACITY)) = sz;
    *(anna_entry_t **)anna_entry_get_addr(this,ANNA_MID_LIST_PAYLOAD) = ptr;
}

int anna_list_ensure_capacity(anna_object_t *this, size_t sz)
{
    anna_entry_t *ptr = anna_list_get_payload(this);
    size_t old_cap = (*(size_t *)anna_entry_get_addr(this,ANNA_MID_LIST_CAPACITY));
    size_t old_sz = (*(size_t *)anna_entry_get_addr(this,ANNA_MID_LIST_SIZE));
    
    if(old_cap >= sz)
    {
	return 0;
    }
    size_t cap = anna_size_round(sz);
    
    ptr = realloc(ptr, sizeof(anna_entry_t )*cap);
    if(!ptr)
    {
	return 1;
    }
    size_t i;
    for(i=old_sz; i<sz; i++)
    {
	ptr[i] = null_entry;
    }
    (*(size_t *)anna_entry_get_addr(this,ANNA_MID_LIST_CAPACITY)) = cap;
    (*(size_t *)anna_entry_get_addr(this,ANNA_MID_LIST_SIZE)) = maxi(sz, old_sz);
    *(anna_entry_t **)anna_entry_get_addr(this,ANNA_MID_LIST_PAYLOAD) = ptr;
    return 0;    
}

anna_entry_t *anna_list_get_payload(anna_object_t *this)
{
    return *(anna_entry_t **)anna_entry_get_addr(this,ANNA_MID_LIST_PAYLOAD);
}

ANNA_VM_NATIVE(anna_list_set_int, 3)
{
    ANNA_ENTRY_NULL_CHECK(param[1]);
    anna_list_set(anna_as_obj(param[0]), anna_as_int(param[1]), param[2]);
    return param[2];
}

ANNA_VM_NATIVE(anna_list_flip, 3)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    ANNA_ENTRY_NULL_CHECK(param[1]);
    ANNA_ENTRY_NULL_CHECK(param[2]);
    anna_object_t *obj = anna_as_obj(param[0]);
    int idx1 = anna_as_int(param[1]);
    int idx2 = anna_as_int(param[2]);
    anna_entry_t tmp = anna_list_get(obj, idx1);
    anna_list_set(obj, idx1, anna_list_get(obj, idx2));
    anna_list_set(obj, idx2, tmp);
    return param[2];
}

ANNA_VM_NATIVE(anna_list_get_int, 2)
{ 
    ANNA_ENTRY_NULL_CHECK(param[0]);
    ANNA_ENTRY_NULL_CHECK(param[1]);
    return anna_list_get(anna_as_obj(param[0]), anna_as_int(param[1]));
}

ANNA_VM_NATIVE(anna_list_get_count_method, 1)
{
    return anna_from_int(anna_list_get_count(anna_as_obj(param[0])));
}

ANNA_VM_NATIVE(anna_list_get_iterator, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *list = anna_as_obj(param[0]);
    anna_object_t *iter = anna_object_create(
	anna_type_unwrap(anna_entry_get_static_obj(list->type,ANNA_MID_ITERATOR_TYPE)));
    *anna_entry_get_addr(iter, ANNA_MID_COLLECTION) = param[0];
    *anna_entry_get_addr(iter, ANNA_MID_KEY) = anna_from_int(0);    
    *anna_entry_get_addr(iter, ANNA_MID_STEP) = anna_from_int(1);    
    return anna_from_obj(iter);
}

ANNA_VM_NATIVE(anna_list_get_iterator_reverse, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *list = anna_as_obj(param[0]);
    anna_object_t *iter = anna_object_create(
	anna_type_unwrap(anna_entry_get_static_obj(list->type,ANNA_MID_ITERATOR_TYPE)));
    *anna_entry_get_addr(iter, ANNA_MID_COLLECTION) = param[0];
    *anna_entry_get_addr(iter, ANNA_MID_KEY) = anna_from_int(anna_list_get_count(list)-1);
    *anna_entry_get_addr(iter, ANNA_MID_STEP) = anna_from_int(-1);
    return anna_from_obj(iter);
}

ANNA_VM_NATIVE(anna_list_empty, 1)
{
    return anna_list_get_count(anna_as_obj(param[0])) ? null_entry : anna_from_int(1);
}

ANNA_VM_NATIVE(anna_list_get_first, 1)
{
    return anna_list_get(anna_as_obj(param[0]), 0);
}

ANNA_VM_NATIVE(anna_list_get_last, 1)
{
    return anna_list_get(anna_as_obj(param[0]), anna_list_get_count(anna_as_obj(param[0]))-1);
}

ANNA_VM_NATIVE(anna_list_set_count_method, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[1]);
    int sz = anna_as_int(param[1]);
    anna_list_set_count(anna_as_obj(param[0]), sz);
    return param[1];
}

ANNA_VM_NATIVE(anna_list_clear, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_list_set_count(anna_as_obj(param[0]), 0);
    return param[0];
}

ANNA_VM_NATIVE(anna_list_append, 2)
{
    size_t i;

    size_t capacity = anna_list_get_capacity(anna_as_obj(param[0]));
    size_t size = anna_list_get_count(anna_as_obj(param[0]));
    size_t size2 = anna_list_get_count(anna_as_obj(param[1]));
    size_t new_size = size+size2;
    
    if(capacity <= (new_size))
    {
	anna_list_set_capacity(anna_as_obj(param[0]), maxi(8, new_size*2));
    }
    anna_entry_t *ptr = anna_list_get_payload(anna_as_obj(param[0]));
    anna_entry_t *ptr2 = anna_list_get_payload(anna_as_obj(param[1]));
    *(size_t *)anna_entry_get_addr(anna_as_obj(param[0]),ANNA_MID_LIST_SIZE) = new_size;
    for(i=0; i<size2; i++)
    {
	ptr[size+i]=ptr2[i];
    }
    
    return param[0];
}

ANNA_VM_NATIVE(anna_list_init, 2)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_object_t *that = anna_as_obj_fast(param[1]);

    size_t sz = anna_list_get_count(that);
    anna_entry_t *src = anna_list_get_payload(that);
    
    anna_list_set_count(this, sz);
    anna_entry_t *dest = anna_list_get_payload(this);
    memcpy(dest, src, sizeof(anna_object_t *)*sz);
    
    return param[0];
}

static void anna_list_del(anna_object_t *victim)
{
    free(anna_entry_get_ptr(victim,ANNA_MID_LIST_PAYLOAD));
    (*(size_t *)anna_entry_get_addr(victim,ANNA_MID_LIST_CAPACITY)) = 0;    
    (*(size_t *)anna_entry_get_addr(victim,ANNA_MID_LIST_SIZE)) = 0;
}

ANNA_VM_NATIVE(anna_list_pop, 1)
{
    
    size_t *sz = (size_t *)anna_entry_get_addr(anna_as_obj_fast(param[0]),ANNA_MID_LIST_SIZE);
    if(!*sz)
	return null_entry;
    (*sz)--;
    return (*(anna_entry_t **)anna_entry_get_addr(anna_as_obj_fast(param[0]),ANNA_MID_LIST_PAYLOAD))[*sz];
}

static void anna_list_in_callback(anna_context_t *context)
{
    anna_entry_t ret = anna_context_pop_entry(context);
//    anna_message(L"Wee, in callback value: %ls\n", ret==null_object?L"null":L"not null");
    anna_entry_t *param = context->top - 3;
    anna_object_t *list = anna_as_obj_fast(param[0]);
    anna_object_t *value = anna_as_obj_fast(param[1]);
    int idx = anna_as_int(param[2]);
    size_t sz = anna_list_get_count(list);
    
    if(!anna_entry_null(ret) && (anna_as_int(ret) == 0))
    {
	anna_context_drop(context, 4);
	anna_context_push_entry(context, anna_from_int(idx-1));
    }
    else if(sz > idx)
    {
	anna_entry_t o_param[] =
	    {
		anna_from_obj(value),
		anna_list_get(list, idx)
	    }
	;
	
	anna_object_t *fun_object = anna_as_obj_fast(anna_entry_get_static(value->type, ANNA_MID_CMP));
	param[2] = anna_from_int(idx+1);
	anna_vm_callback_reset(context, fun_object, 2, o_param);
    }
    else
    {
	anna_context_drop(context, 4);
	anna_context_push_object(context, null_object);
    }
}

static void anna_list_in(anna_context_t *context)
{
    anna_object_t *value = anna_context_pop_object(context);
    anna_object_t *list = anna_context_pop_object(context);
    anna_context_pop_entry(context);
    
    size_t sz = anna_list_get_count(list);
    
    if(sz > 0)
    {
	anna_entry_t callback_param[] = 
	    {
		anna_from_obj(list),
		anna_from_obj(value),
		anna_from_int(1),
	    }
	;
	
	anna_entry_t o_param[] =
	    {
		anna_from_obj(value),
		anna_list_get(list, 0)
	    }
	;
	
	anna_object_t *fun_object = anna_as_obj_fast(anna_entry_get_static(value->type, ANNA_MID_CMP));
	anna_vm_callback_native(
	    context,
	    anna_list_in_callback, 3, callback_param,
	    fun_object, 2, o_param
	    );
    }
    else
    {
	anna_context_push_object(context, null_object);
    }
}

ANNA_VM_NATIVE(anna_list_i_get_range, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[1]);
    anna_object_t *list = anna_as_obj_fast(param[0]);
    size_t count = anna_list_get_count(list);
    anna_object_t *range = anna_as_obj(param[1]);

    int from = anna_idx_wrap(anna_range_get_from(range), count);
    int to = anna_idx_wrap(anna_range_get_to(range), count);
    int step = anna_range_get_step(range);
    int i;

    if(anna_range_get_open(range))
    {
	to = (step>0) ? count : -1;
    }
    else if((to >= from) != (step > 0))
    {
	step = -step;
    }

    if((from < 0) || (from >= count))
    {
	return null_entry;
    }

    ssize_t count_slice = (1+(to-from-sign(step))/step);
    ssize_t last_idx = from + step*(count_slice-1);
    if((last_idx < 0) || (last_idx >= count))
    {
	return null_entry;
    }
    
    anna_object_t *res = anna_list_is_mutable(list)?
	anna_list_create_mutable(anna_list_get_specialization(list)):
	anna_list_create_imutable(anna_list_get_specialization(list));

    for(i=from;(step>0)? i<to : i>to; i+=step)
    {
	anna_list_push(
	    res, 
	    anna_list_get(
		list,
		i));
    }
    
    return anna_from_obj(res);    
}

ANNA_VM_NATIVE(anna_list_i_set_range, 3)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    ANNA_ENTRY_NULL_CHECK(param[1]);
    
    anna_object_t *replacement;
    anna_object_t *range = anna_as_obj(param[1]);
    anna_object_t *list = anna_as_obj(param[0]);
    
    if(anna_entry_null(param[2]))
	replacement = anna_list_create_mutable(any_type);
    else
	replacement = anna_as_obj(param[2]);
    
    size_t size = anna_list_get_count(list);
    int from_orig = anna_range_get_from(range);
    int to_orig = anna_range_get_to(range);

    int from = anna_idx_wrap(from_orig, size);
    int step = anna_range_get_step(range);
    
    int to = anna_idx_wrap(to_orig, size);
    int count;
    int i;

    if(from < 0)
    {
	return null_entry;
    }    

    if(anna_range_get_open(anna_as_obj_fast(param[1])))
    {
	to = step>0?anna_list_get_count(list):-1;
    }
    else
    {
	if(from_orig < 0 || to_orig < 0)
	{
	    if((to > from) != (step > 0))
	    {
		step = -step;
	    }
	}
    }
    
    count = (1+(to-from-sign(step))/step);
    
    int count_replacement = anna_list_get_count(replacement);

    if(count_replacement == 0)
    {
	/*
	  Erase mode.

	  The replacement list is either empty or null. In this case,
	  we remove all the elements in the specified slice from the
	  list.
	 */

	if(to < from)
	{
	    /*
	      If this is a decreasing range, reverse it. It's easier to
	      deal with just one direction.
	      
	      Note that the -1 in the first line here comes from the fact
	      that the first item of a range is inclusive and the last one
	      is exclusive.
	    */
	    from = from + step*(count-1);
	    step = -step;
	    to = from + step*count;
	}
	
	int out = from;
	anna_entry_t *arr = anna_list_get_payload(list);
	int in;
	int old_size = anna_list_get_count(list);
	int new_size = maxi(0, old_size - count);

	if(to!=old_size || step!=1)
	{
	    for(in=from; in < old_size; in++)
	    {
		if((in >= to) || (in-from)%step != 0)
		{
		    arr[out++] = arr[in];
		}
	    }
	    *(size_t *)anna_entry_get_addr(list,ANNA_MID_LIST_SIZE) = out;
	}
	else
	{
	    *(size_t *)anna_entry_get_addr(list,ANNA_MID_LIST_SIZE) = new_size;
	}
	
    }
    else if(count != count_replacement)
    {
	/*
	  Complex replace mode. 

	  In this mode, we're replacing a slice with a replacement
	  list of a different length. This mode will only work if the
	  slice has a step of +-1.

	 */
	if(step*step != 1 && count != 0)
	{
	    return null_entry;
	}
	if(count == 0)
	{
	    step=1;
	}
	
	int old_size = anna_list_get_count(list);

	/* If we're assigning past the end of the array, just silently
	 * take the whole array and go on */
	count = mini(count, old_size - from);
	int new_size = old_size - count + count_replacement;
	anna_entry_t *arr;
	if(new_size > anna_list_get_capacity(list))
	{
	    anna_list_set_capacity(list, new_size);
	}

	arr = anna_list_get_payload(list);

	for(i=old_size; i<new_size; i++)
	{
	    arr[i] = null_entry;
	}

	/* Set new size - don't call anna_list_set_count, since that might truncate the list if we're shrinking */
	*(size_t *)anna_entry_get_addr(list,ANNA_MID_LIST_SIZE) = new_size;
	/* Move the old data */
	memmove(&arr[mini(from,to)+count_replacement], &arr[mini(from,to)+count], sizeof(anna_object_t *)*abs(old_size - mini(from,to) - count ));

	/* Copy in the new data */
	int offset = (step > 0) ? (from) : (from+count_replacement-count);
	for(i=0;i<count_replacement;i++)
	{
	    arr[offset+step*i/*+count_replacement-count*/] = 
		anna_list_get(
		    replacement,
		    i);
	}
    }
    else
    {
	/*
	  Simple replace mode. This only works if the number of items
	  in the slice and replacement are equal. In the simple mode,
	  we can simply iterate over the indices in the slice and
	  replace them with the corresponding item from the
	  replacement.
	*/
	for(i=0;i<count;i++)
	{
	    anna_list_set(
		list, from + step*i, 
		anna_list_get(
		    replacement,
		    i));
	}
    }

    return param[0];
}

ANNA_VM_NATIVE(anna_list_i_copy_imutable, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_object_t *that = anna_list_create_imutable(anna_list_get_specialization(this));
    anna_list_set_count(that, anna_list_get_count(this));      
    memcpy(
	anna_list_get_payload(that),
	anna_list_get_payload(this),
	sizeof(anna_entry_t ) * anna_list_get_count(this));
    
    return anna_from_obj(that);
}

ANNA_VM_NATIVE(anna_list_i_copy_mutable, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_object_t *that = anna_list_create_mutable(anna_list_get_specialization(this));
    anna_list_set_count(that, anna_list_get_count(this));      
    memcpy(
	anna_list_get_payload(that),
	anna_list_get_payload(this),
	sizeof(anna_entry_t ) * anna_list_get_count(this));
    
    return anna_from_obj(that);
}

ANNA_VM_NATIVE(anna_list_i_join, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    ANNA_ENTRY_NULL_CHECK(param[1]);
    anna_object_t *this = anna_as_obj(param[0]);
    anna_object_t *other = anna_as_obj(param[1]);
    anna_object_t *res = anna_list_create2(this->type);
    size_t c1 = anna_list_get_count(this);
    size_t c2 = anna_list_get_count(other);
    anna_list_set_capacity(res, c1+c2);
    int i;
    
    for(i=0; i<c1; i++)
    {
	anna_list_set(res, i, anna_list_get(this, i));
    }
    
    for(i=0; i<c2; i++)
    {
	anna_list_set(res, i+c1, anna_list_get(other, i));
    }
    
    return anna_from_obj(res);
    
}

ANNA_VM_NATIVE(anna_list_iterator_get_value, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *iter = anna_as_obj(param[0]);
    anna_object_t *list = anna_as_obj(anna_entry_get(iter, ANNA_MID_COLLECTION));
    int offset = anna_as_int(anna_entry_get(iter, ANNA_MID_KEY));
    return anna_list_get(list, offset);
}

ANNA_VM_NATIVE(anna_list_iterator_set_value, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *iter = anna_as_obj(param[0]);
    anna_object_t *list = anna_as_obj(anna_entry_get(iter, ANNA_MID_COLLECTION));
    int offset = anna_as_int(anna_entry_get(iter, ANNA_MID_KEY));
    anna_list_set(list, offset, param[1]);
    return param[1];
}

ANNA_VM_NATIVE(anna_list_iterator_valid, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *iter = anna_as_obj(param[0]);
    anna_object_t *list = anna_as_obj(anna_entry_get(iter, ANNA_MID_COLLECTION));
    int offset = anna_as_int(anna_entry_get(iter, ANNA_MID_KEY));
    return (offset >= 0 && offset < anna_list_get_count(list)) ? anna_from_int(1) : null_entry;
}

ANNA_VM_NATIVE(anna_list_iterator_next, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *iter = anna_as_obj(param[0]);
    anna_entry_set(iter, ANNA_MID_KEY, anna_from_int(anna_as_int(anna_entry_get(iter, ANNA_MID_STEP))+anna_as_int(anna_entry_get(iter, ANNA_MID_KEY))));
    return param[0];
}

static anna_type_t *anna_list_iterator_create(
    anna_type_t *type,
    anna_type_t *spec,
    int mutable)
{
    anna_type_t *iter = anna_type_create(L"Iterator", 0);
    
    anna_member_create(
	iter, ANNA_MID_COLLECTION, ANNA_MEMBER_IMUTABLE, type);
    anna_member_create(
	iter, ANNA_MID_KEY, ANNA_MEMBER_IMUTABLE, int_type);    
    anna_member_create(
	iter, ANNA_MID_STEP, ANNA_MEMBER_IMUTABLE, int_type);
    anna_type_copy_object(iter);

    anna_member_create_native_property(
	iter, ANNA_MID_VALUE, spec,
	&anna_list_iterator_get_value,
	mutable ? &anna_list_iterator_set_value:0, 0);

    anna_member_create_native_property(
	iter, ANNA_MID_VALID, any_type,
	&anna_list_iterator_valid,
	0, 0);
    
    anna_type_t *iter_argv[] = 
	{
	    iter
	}
    ;
    
    wchar_t *iter_argn[] = 
	{
	    L"this"
	}
    ;
    
    anna_member_create_native_method(
	iter,
	ANNA_MID_NEXT_ASSIGN, 0,
	&anna_list_iterator_next, iter, 1,
	iter_argv, iter_argn, 0, 0);

    anna_util_iterator_iterator(iter);
        
    anna_type_close(iter);
    return iter;
}

static void anna_list_type_create_internal(
    anna_type_t *type, 
    anna_type_t *spec,
    anna_type_t *imutable_type, 
    anna_type_t *mutable_type, 
    anna_type_t *intersection_type,
    int mutable)
{

    anna_type_t *a_argv[] = 
	{
	    type,
	    spec
	}
    ;
    
    wchar_t *a_argn[]=
	{
	    L"this", L"value"
	}
    ;

    anna_type_t *o_argv[] = 
	{
	    type,
	    any_type
	}
    ;

    anna_type_t *l_argv[] = 
	{
	    type,
	    intersection_type
	}
    ;
    
    wchar_t *l_argn[]=
	{
	    L"this", L"value"
	}
    ;

    anna_type_set_initializer(type, &anna_list_type_init);

    anna_member_create(
	type,
	ANNA_MID_ITERATOR_TYPE,
	ANNA_MEMBER_STATIC,
	type_type);

    anna_type_t *iter = anna_list_iterator_create(
	type,
	spec,
	mutable);
    
    anna_entry_set_static(
	 type, ANNA_MID_ITERATOR_TYPE,
	 anna_from_obj(anna_type_wrap(iter)));

    anna_member_create(
	type, ANNA_MID_LIST_PAYLOAD, 0, null_type);

    anna_member_create(
	type,
	ANNA_MID_LIST_SIZE,
	0,
	null_type);

    anna_member_create(
	type,
	ANNA_MID_LIST_CAPACITY,
	0,
	null_type);

    anna_member_create(
	type,
	ANNA_MID_LIST_SPECIALIZATION,
	ANNA_MEMBER_STATIC,
	null_type);
    anna_entry_set_static_obj(type, ANNA_MID_LIST_SPECIALIZATION, (anna_object_t *)spec);
    
    anna_member_create_native_method(
	type, ANNA_MID_INIT,
	ANNA_FUNCTION_VARIADIC, &anna_list_init,
	type, 2, a_argv, a_argn, 0, 0);
    
    anna_type_finalizer_add(
	type, anna_list_del);
        
    anna_type_t *i_argv[] = 
	{
	    type,
	    int_type,
	    spec
	}
    ;

    wchar_t *i_argn[]=
	{
	    L"this", L"index", L"value"
	}
    ;

    anna_member_create_native_method(
	type,
	ANNA_MID_GET, 0,
	&anna_list_get_int, spec, 2,
	i_argv, i_argn, 0, 0);
    anna_member_alias(type, ANNA_MID_GET, L"__get__");

    anna_member_create_native_property(
	type, ANNA_MID_COUNT, int_type,
	&anna_list_get_count_method,
	mutable ? &anna_list_set_count_method : 0,
	L"The number of elements in this list.");

    anna_member_create_native_property(
	type, ANNA_MID_ITERATOR, iter,
	&anna_list_get_iterator, 0,
	L"Returns an Iterator for this collection.");

    anna_member_create_native_property(
	type, ANNA_MID_REVERSE, iter,
	&anna_list_get_iterator_reverse, 0,
	L"Returns an Iterator for this collection that will iterate over the element from the end of the list and forward.");

    anna_member_create_native_property(
	type, ANNA_MID_EMPTY, int_type,
	&anna_list_empty,
	0,
	L"This property is true (non-null) if the list is empty, null otherwise.");

    wchar_t *join_argn[] =
	{
	    L"this", L"other"
	}
    ;
    
    anna_type_t *join_argv[] = 
	{
	    type,
	    intersection_type
	}
    ;

    anna_type_t *switch_argv[] = 
	{
	    type,
	    int_type,
	    int_type
	}
    ;

    wchar_t *switch_argn[] =
	{
	    L"this", L"index1", L"index2"
	}
    ;

    anna_member_create_native_method(
	type, ANNA_MID_JOIN_OP, 0,
	&anna_list_i_join, type,
	2, join_argv, join_argn, 0, L"The join operator is used to create a new list that contains all the elements of both the original lists.");
    anna_member_document(
	type, ANNA_MID_JOIN_OP,
	L"The join operator can be used like this:");
    anna_member_document_example(
	type, ANNA_MID_JOIN_OP, 
	L"combinedCollection := firstCollection ~ secondCollection;");
    anna_member_document(
	type, ANNA_MID_JOIN_OP,
	L"Note that the join operator joins two collections. If all you want to do is add one element to an existing collection, use the <a href='#push'>push method</a>.");
  

    anna_member_create_native_property(
	type,
	ANNA_MID_FIRST,
	spec,
	&anna_list_get_first, 0,
	L"The first element of this list.");

    anna_member_create_native_property(
	type,
	ANNA_MID_LAST,
	spec,
	&anna_list_get_last,
	0,
	L"The last element of this list");

    anna_member_create_native_method(
	type, anna_mid_get(L"__appendAssign__"),
	0, &anna_list_append, type, 2, l_argv,
	l_argn, 0, 0);

    anna_member_create_native_method(
	type,
	ANNA_MID_IN,
	0,
	&anna_list_in,
	int_type,
	2,
	o_argv,
	a_argn, 0, 
	L"If the specified value exists in the list, the first index where it can be found is returned, otherwise, null is returned.");
    
    anna_type_t *range_argv[] = 
	{
	    type,
	    range_type,
	    type
	}
    ;

    wchar_t *range_argn[] =
	{
	    L"this", L"range", L"value"
	}
    ;

    anna_member_create_native_method(
	type,
	ANNA_MID_GET_RANGE, 0,
	&anna_list_i_get_range, type, 2,
	range_argv, range_argn, 0, L"Returns a newly created List containing all the elements in the specified Range.");
    anna_member_document_example(type, ANNA_MID_GET_RANGE, L"myList := [1,2,3,4,5,6];\nprint(myList[1|2...]); // This will print [2, 4, 6]");
    anna_member_alias(type, ANNA_MID_GET_RANGE, L"__get__");

    if(mutable)
    {	
	anna_member_create_native_method(
	    type,
	    ANNA_MID_SET, 0,
	    &anna_list_set_int, spec, 3,
	    i_argv, i_argn, 0, 0);
	anna_member_alias(type, ANNA_MID_SET, L"__set__");

	anna_member_create_native_method(
	    type,
	    anna_mid_get(L"flip"), 0,
	    &anna_list_flip, spec, 3,
	    switch_argv, switch_argn, 0, L"Flip the values at the two specified indices.");

	anna_member_create_native_method(
	    type, ANNA_MID_PUSH,
	    ANNA_FUNCTION_VARIADIC, &anna_list_append,
	    type,
	    2,
	    a_argv,
	    a_argn, 0, 
	    L"Adds the specified element to the end of the list. Returns the mutated list.");
	
	anna_member_create_native_method(
	    type,
	    ANNA_MID_CLEAR, 0,
	    &anna_list_clear, type, 1,
	    i_argv, i_argn, 0, L"Clear the List, removing all entries from it. The result is an empty List.");

	anna_member_create_native_method(
	    type, ANNA_MID_POP, 0,
	    &anna_list_pop, spec, 1, a_argv, a_argn, 0, 
	    L"Removes the last element from the list and returns it. Returns null if the list is already empty.");
	
	anna_member_create_native_method(
	    type,
	    ANNA_MID_SET_RANGE, 0,
	    &anna_list_i_set_range, type, 3,
	    range_argv, range_argn, 0, L"Assign new values to a range of items.");
	
	anna_member_document(
	    type, ANNA_MID_SET_RANGE,
	    L"This method can operate in three distinct modes:");
	
	anna_member_document(
	    type, ANNA_MID_SET_RANGE,
	    L"The first mode, replacement mode, is when the slice to replace and the supplied replacement have the same number of items. In this case, the slice operation will simply replace the old values with the new ones in place and the list will be otherwise unchanged. Some examples of simple mode usage:");
	
	anna_member_document_example(
	    type, ANNA_MID_SET_RANGE,
	    L"// Simple replacement\n"
	    L"[1,2,3,4,5][1..2] = [22, 33]; // Result: [1, 22, 33, 4, 5]\n"
	    L"// In simple mode, it is allowed to use any step in the range. This\n// will replace the even items\n"
	    L"[1,2,3,4,5][1|2..4] = [22, 44]; // Result: [1, 22, 3, 44, 5]\n"
	    L"// We can use open ranges, and the end of the range will simply be\n// the list boundary. This will replace the even items.\n"
	    L"[1,2,3,4,5][1|2...] = [22, 44]; // Result: [1, 22, 3, 44, 5]\n"
	    );

	anna_member_document(
	    type, ANNA_MID_SET_RANGE,
	    L"The second mode, splice mode, is when the slice to replace and the supplied replacement have different lengths. In complex mode, the step of the slice range must always be ±1. In this case, the slice operation will remove the old range and replace them with new values, which results in a list with a different length.");
	anna_member_document_example(
	    type, ANNA_MID_SET_RANGE,
	    L"// Complex replacement, change the list length\n"
	    L"[1,2,3,4,5][1..2] = [11, 22, 33, 44]; // Result: [1, 11, 22, 33, 44, 4, 5]\n"
	    L"// In complex mode, it is not allowed to use any step in the range.\n"
	    L"[1,2,3,4,5][1|2..4] = [11, 22, 33, 44]; // This will result in an error\n"
	    L"// We can use open ranges, and the end of the range will simply be\n// the list boundary. This will replace the last element with the new list.\n"
	    L"[1,2,3,4,5][-1...] = [11,22,33,44]; // Result: [1, 2, 3, 4, 11, 22, 33, 44]\n"
	    );

	anna_member_document(
	    type, ANNA_MID_SET_RANGE,
	    L"The final mode, erase mode, is when the supplied replacement is null or has a length of zero. In this case, all those elements will be removed from the list, and the list will thus be shortened."
	    );
	anna_member_document_example(
	    type, ANNA_MID_SET_RANGE,
	    L"// Erase mode, change the list length\n"
	    L"[1,2,3,4,5][1..2] = «Int»[]; // Result: [1, 3, 4, 5]\n"
	    L"// Same as above, but using a null value instead of an empty list\n"	    
	    L"[1,2,3,4,5][1..2] = ?; // Result: [1, 3, 4, 5]\n"
	    L"// Erase mode, we can use any step in erase mode\n"
	    L"[1,2,3,4,5][1|2..5] = «Int»[]; // Result: [1, 3, 5]\n"
	    L"// Erase mode, open ranges work as exected\n"
	    L"[1,2,3,4,5][1|2...] = «Int»[]; // Result: [1, 3, 5]\n"
	    );

	anna_member_alias(type, ANNA_MID_SET_RANGE, L"__set__");
    }
    
    anna_member_create_native_property(
	type, ANNA_MID_FREEZE,
	imutable_type, mutable ? &anna_list_i_copy_imutable : &anna_util_noop,
	0,
	L"An imutable copy of this List, or the List itself if it is already imutable.");
    
    anna_member_create_native_property(
	type, ANNA_MID_THAW,
	mutable_type, mutable ? &anna_util_noop : &anna_list_i_copy_mutable, 0,
	L"A mutable copy of this List, or the List itself if it is already mutable.");

    
}

static inline void anna_list_internal_init()
{
    static int init = 0;
    if(init)
	return;
    init=1;
    hash_init(&anna_list_specialization, hash_ptr_func, hash_ptr_cmp);
}

static anna_type_t **anna_list_type_insert(
    anna_type_t *subtype, 
    anna_type_t *mutable, anna_type_t *imutable, anna_type_t *any)
{
    anna_type_t **res = malloc(sizeof(anna_type_t *)*3);
    res[MUTABLE_OFF] = mutable;
    res[IMUTABLE_OFF] = imutable;
    res[ANY_OFF] = any;
    
    hash_put(&anna_list_specialization, subtype, res);
    anna_alloc_mark_permanent(mutable);
    anna_alloc_mark_permanent(imutable);
    anna_alloc_mark_permanent(any);
    return res;
}

static anna_type_t *anna_list_specialize_node(anna_type_t *base, anna_node_call_t *call, anna_stack_template_t *stack)
{
    if(call->child_count != 1)
    {
	anna_error((anna_node_t *)call, L"Invalid number of template arguments to list specialization");
	return 0;
    }
    
    call->child[0] = anna_node_calculate_type(call->child[0]);	
    anna_type_t *spec = anna_node_resolve_to_type(call->child[0], stack);

    if(spec)
    {
	return base == mutable_list_type ? 
	    anna_list_type_get_mutable(spec) :
	    (
		base == imutable_list_type ?
		anna_list_type_get_imutable(spec):
		anna_list_type_get_any(spec)
		);
    }
    
    anna_error((anna_node_t *)call, L"List specialization can not be resolved into type");
    return 0;
}

void anna_list_type_create()
{
    anna_list_internal_init();

    anna_list_type_insert(
	any_type, mutable_list_type, imutable_list_type, any_list_type);
    
    anna_list_type_create_internal(
	mutable_list_type, any_type, 
	imutable_list_type, mutable_list_type, any_list_type, 1);
    anna_list_type_create_internal(
	imutable_list_type, any_type, 
	imutable_list_type, mutable_list_type, any_list_type, 0);
    anna_type_intersect_into(
	any_list_type, mutable_list_type, imutable_list_type);    

    mutable_list_type->specialization_function = anna_list_specialize_node;
    imutable_list_type->specialization_function = anna_list_specialize_node;
    any_list_type->specialization_function = anna_list_specialize_node;
    
    anna_entry_set_static_ptr(any_list_type, ANNA_MID_LIST_SPECIALIZATION, any_type);
    anna_list_push_all_extra_methods(any_list_type, ANY_OFF);

    anna_type_document(
	any_list_type,
	L"The List type represents any list, either a mutable or imutable one. It is the intersection of the MutableList and the ImutableList.");
    
    anna_type_document(
	mutable_list_type,
	L"The MutableList type is type representing a mutable (changing) list of objects.");
    
    anna_type_document(
	imutable_list_type,
	L"The ImutableList type is type representing an imutable (unchanging) list of objects.");
}

static anna_type_t **anna_list_type_get_internal(anna_type_t *subtype)
{
    anna_list_internal_init();
    
    anna_type_t **res = hash_get(&anna_list_specialization, subtype);
    if(!res)
    {
	string_buffer_t sb;
	sb_init(&sb);

	sb_printf(&sb, L"MutableList«%ls»", subtype->name);
	anna_type_t *mutable = anna_type_create(sb_content(&sb), 0);

	sb_truncate(&sb, 0);
	sb_printf(&sb, L"ImutableList«%ls»", subtype->name);
	anna_type_t *imutable = anna_type_create(
	    sb_content(&sb), 0);

	if(anna_type_sendable(subtype))
	{
	    anna_type_make_sendable(imutable);
	}
	
	sb_truncate(&sb, 0);
	sb_printf(&sb, L"List«%ls»", subtype->name);
	anna_type_t *any = anna_type_create(
	    sb_content(&sb), 0);
	
	sb_destroy(&sb);

	res = anna_list_type_insert(subtype, mutable, imutable, any);
	
	anna_list_type_create_internal(
	    mutable, subtype, imutable, mutable, any, 1);
	anna_list_type_create_internal(
	    imutable, subtype, imutable, mutable, any, 0);
	anna_type_intersect_into(any, mutable, imutable);

	anna_entry_set_static_ptr(any, ANNA_MID_LIST_SPECIALIZATION, subtype);

	anna_list_push_all_extra_methods(mutable, MUTABLE_OFF);
	anna_list_push_all_extra_methods(any, ANY_OFF);
	anna_list_push_all_extra_methods(imutable, IMUTABLE_OFF);

	anna_type_close(any);
	anna_type_close(mutable);
	anna_type_close(imutable);
	
	mutable->flags |= ANNA_TYPE_SPECIALIZED;
	imutable->flags |= ANNA_TYPE_SPECIALIZED;
	any->flags |= ANNA_TYPE_SPECIALIZED;
	
	anna_type_copy_object(mutable);
	anna_type_copy_object(imutable);
	anna_type_copy_object(any);
    }
    
    return res;
}

anna_type_t *anna_list_type_get_mutable(anna_type_t *subtype)
{
    return anna_list_type_get_internal(subtype)[MUTABLE_OFF];
}

anna_type_t *anna_list_type_get_imutable(anna_type_t *subtype)
{
    return anna_list_type_get_internal(subtype)[IMUTABLE_OFF];
}

anna_type_t *anna_list_type_get_any(anna_type_t *subtype)
{
    return anna_list_type_get_internal(subtype)[ANY_OFF];
}

