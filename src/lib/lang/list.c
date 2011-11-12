
#define MUTABLE_OFF 0
#define IMUTABLE_OFF 1
#define ANY_OFF 2

anna_type_t *mutable_list_type = 0;
anna_type_t *imutable_list_type = 0;
anna_type_t *any_list_type = 0;
static hash_table_t anna_list_specialization;
static array_list_t anna_list_additional_methods = AL_STATIC;

static void add_list_method(void *key, void *value, void *aux)
{
    anna_type_t **list = (anna_type_t **)value;
    anna_function_t *fun = (anna_function_t *)aux;
//    wprintf(L"Add function %ls to type %ls\n", fun->name, list->name);
    anna_member_create_method(list[0], anna_mid_get(fun->name), fun);
    anna_member_create_method(list[1], anna_mid_get(fun->name), fun);
    anna_member_create_method(list[2], anna_mid_get(fun->name), fun);
}

void anna_list_add_method(anna_function_t *fun)
{
//    wprintf(L"Function %ls to all list types\n", fun->name);
    al_push(&anna_list_additional_methods, fun);
    hash_foreach2(&anna_list_specialization, &add_list_method, fun);
}

static void anna_list_add_all_extra_methods(anna_type_t *list)
{
    int i;
    for(i=0; i<al_get_count(&anna_list_additional_methods); i++)
    {
	anna_function_t *fun = (anna_function_t *)al_get(&anna_list_additional_methods, i);
//	wprintf(L"Add function %ls to type %ls\n", fun->name, list->name);
	anna_member_create_method(list, anna_mid_get(fun->name), fun);
    }
}

anna_object_t *anna_list_create_imutable(anna_type_t *spec)
{
    anna_object_t *obj= anna_object_create(anna_list_type_get_imutable(spec));
    (*anna_entry_get_addr(obj,ANNA_MID_LIST_PAYLOAD))=0;
    (*(size_t *)anna_entry_get_addr(obj,ANNA_MID_LIST_CAPACITY)) = 0;    
    (*(size_t *)anna_entry_get_addr(obj,ANNA_MID_LIST_SIZE)) = 0;
    obj->flags |= ANNA_OBJECT_LIST;
    return obj;
}

anna_object_t *anna_list_create_mutable(anna_type_t *spec)
{
    anna_object_t *obj= anna_object_create(anna_list_type_get_mutable(spec));
    (*anna_entry_get_addr(obj,ANNA_MID_LIST_PAYLOAD))=0;
    (*(size_t *)anna_entry_get_addr(obj,ANNA_MID_LIST_CAPACITY)) = 0;    
    (*(size_t *)anna_entry_get_addr(obj,ANNA_MID_LIST_SIZE)) = 0;
    obj->flags |= ANNA_OBJECT_LIST;
    return obj;
}

anna_object_t *anna_list_create2(anna_type_t *list_type)
{
    anna_object_t *obj= anna_object_create(list_type);
    (*anna_entry_get_addr(obj,ANNA_MID_LIST_PAYLOAD))=0;
    (*(size_t *)anna_entry_get_addr(obj,ANNA_MID_LIST_CAPACITY)) = 0;    
    (*(size_t *)anna_entry_get_addr(obj,ANNA_MID_LIST_SIZE)) = 0;
    obj->flags |= ANNA_OBJECT_LIST;
    return obj;
}

static anna_type_t *anna_list_get_specialization(anna_object_t *obj)
{
    return *((anna_type_t **)
	     anna_entry_get_addr(
		 obj,
		 ANNA_MID_LIST_SPECIALIZATION));    
}

static int anna_list_is_mutable(anna_object_t *obj)
{
    return obj->type->name[0] == L'M';
}

void anna_list_set(struct anna_object *this, ssize_t offset, anna_entry_t *value)
{
    size_t size = anna_list_get_count(this);
    ssize_t pos = anna_list_calc_offset(offset, size);
    //wprintf(L"Set el %d in list of %d elements\n", pos, size);
    if(pos < 0)
    {
	return;
    }
    if(pos >= size)
    {
	anna_list_set_count(this, pos+1);      
    }
    
    anna_entry_t **ptr = anna_list_get_payload(this);
    ptr[pos] = value;  
}

anna_entry_t *anna_list_get(anna_object_t *this, ssize_t offset)
{
    if(this->type->mid_identifier[ANNA_MID_LIST_PAYLOAD]->offset == 0)
    {
	size_t size = (ssize_t)this->member[1];
	ssize_t pos = anna_list_calc_offset(offset, size);
	anna_entry_t **ptr = (anna_entry_t **)this->member[0];
	if(pos >=size || pos < 0)
	{
	    return null_entry;
	}
	return ptr[pos];
    }
    
    size_t size = anna_list_get_count(this);
    ssize_t pos = anna_list_calc_offset(offset, size);
    anna_entry_t **ptr = anna_list_get_payload(this);
    if(pos < 0||pos >=size)
    {
	return null_entry;
    }
    return ptr[pos];
}

void anna_list_add(struct anna_object *this, anna_entry_t *value)
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
	anna_entry_t **ptr = anna_list_get_payload(this);
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
    anna_entry_t **ptr = anna_list_get_payload(this);
    ptr = realloc(ptr, sizeof(anna_entry_t *)*sz);
    if(!ptr)
    {
	CRASH;
    }    
    (*(size_t *)anna_entry_get_addr(this,ANNA_MID_LIST_CAPACITY)) = sz;
    *(anna_entry_t ***)anna_entry_get_addr(this,ANNA_MID_LIST_PAYLOAD) = ptr;
}

int anna_list_ensure_capacity(anna_object_t *this, size_t sz)
{
    anna_entry_t **ptr = anna_list_get_payload(this);
    size_t old_cap = (*(size_t *)anna_entry_get_addr(this,ANNA_MID_LIST_CAPACITY));
    size_t old_sz = (*(size_t *)anna_entry_get_addr(this,ANNA_MID_LIST_SIZE));
    
    if(old_cap >= sz)
    {
	return 0;
    }
    size_t cap = anna_size_round(sz);
    
    ptr = realloc(ptr, sizeof(anna_entry_t *)*cap);
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
    *(anna_entry_t ***)anna_entry_get_addr(this,ANNA_MID_LIST_PAYLOAD) = ptr;
    return 0;    
}

anna_entry_t **anna_list_get_payload(anna_object_t *this)
{
    return *(anna_entry_t ***)anna_entry_get_addr(this,ANNA_MID_LIST_PAYLOAD);
}

ANNA_VM_NATIVE(anna_list_set_int, 3)
{
    ANNA_ENTRY_NULL_CHECK(param[1]);
    anna_list_set(anna_as_obj(param[0]), anna_as_int(param[1]), param[2]);
    return param[2];
}

ANNA_VM_NATIVE(anna_list_get_int, 2)
{ 
    ANNA_ENTRY_NULL_CHECK(param[1]);
    return anna_list_get(anna_as_obj(param[0]), anna_as_int(param[1]));
}

ANNA_VM_NATIVE(anna_list_get_count_method, 1)
{
    return anna_from_int(anna_list_get_count(anna_as_obj(param[0])));
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
    anna_entry_t **ptr = anna_list_get_payload(anna_as_obj(param[0]));
    anna_entry_t **ptr2 = anna_list_get_payload(anna_as_obj(param[1]));
    *(size_t *)anna_entry_get_addr(anna_as_obj(param[0]),ANNA_MID_LIST_SIZE) = new_size;
    for(i=0; i<size2; i++)
    {
	ptr[size+i]=ptr2[i];
    }
    
    return param[0];
}

/**
   This is the bulk of the each method
 */
static void anna_list_each_callback(anna_context_t *stack)
{
    // Discard the output of the previous method call
    anna_context_pop_entry(stack);
    // Set up the param list. These are the values that aren't reallocated each lap
    anna_entry_t **param = stack->top - 3;
    // Unwrap and name the params to make things more explicit
    anna_object_t *list = anna_as_obj(param[0]);
    anna_object_t *body = anna_as_obj(param[1]);
    int idx = anna_as_int(param[2]);
    size_t sz = anna_list_get_count(list);
    
    // Are we done or do we need another lap?
    if( (idx < sz) && !(stack->frame->flags & ANNA_ACTIVATION_FRAME_BREAK))
    {
	// Set up params for the next lap of the each body function
	anna_entry_t *o_param[] =
	    {
		param[2],
		anna_list_get(list, idx)
	    }
	;
	// Then update our internal lap counter
	param[2] = anna_from_int(idx+1);
	
	// Finally, roll the code point back a bit and push new arguments
	anna_vm_callback_reset(stack, body, 2, o_param);
    }
    else
    {
	// Oops, we're done. Drop our internal param list and push the correct output
	anna_context_drop(stack, 4);
	anna_context_push_object(stack, list);
    }
}

static void anna_list_each(anna_context_t *stack)
{
    anna_object_t *body = anna_context_pop_object(stack);
    anna_object_t *list = anna_context_pop_object(stack);
    anna_context_pop_entry(stack);
    size_t sz = anna_list_get_count(list);

    if(sz > 0)
    {
	anna_entry_t *callback_param[] = 
	    {
		anna_from_obj(list),
		anna_from_obj(body),
		anna_from_int(1)
	    }
	;
	
	anna_entry_t *o_param[] =
	    {
		anna_from_int(0),
		anna_list_get(list, 0)
	    }
	;
	
	anna_vm_callback_native(
	    stack,
	    anna_list_each_callback, 3, callback_param,
	    body, 2, o_param
	    );
    }
    else
    {
	anna_context_push_object(stack, list);
    }
}

static void anna_list_map_callback(anna_context_t *stack)
{
    
    anna_entry_t *value = anna_context_pop_entry(stack);
    
    anna_entry_t **param = stack->top - 4;
    anna_object_t *list = anna_as_obj_fast(param[0]);
    anna_object_t *body = anna_as_obj_fast(param[1]);

    int idx = anna_as_int(param[2]);
    anna_object_t *res = anna_as_obj_fast(param[3]);
    size_t sz = anna_list_get_count(list);

    anna_list_set(res, idx-1, value);

    if( (sz > idx) && !(stack->frame->flags & ANNA_ACTIVATION_FRAME_BREAK))
    {
	anna_entry_t *o_param[] =
	    {
		param[2],
		anna_list_get(list, idx)
	    }
	;
	
	param[2] = anna_from_int(idx+1);
	anna_vm_callback_reset(stack, body, 2, o_param);
    }
    else
    {
	anna_context_drop(stack, 5);
	anna_context_push_object(stack, res);
    }    
}

static void anna_list_map(anna_context_t *stack)
{
    anna_object_t *body = anna_context_pop_object(stack);
    anna_object_t *list = anna_context_pop_object(stack);
    anna_context_pop_entry(stack);
    if(body == null_object)
    {
	anna_context_push_object(stack, null_object);
    }
    else
    {
	anna_function_t *fun = anna_function_unwrap(body);
		
	anna_object_t *res = anna_list_is_mutable(list)?
	    anna_list_create_mutable(fun->return_type):
	    anna_list_create_imutable(fun->return_type);
	
	size_t sz = anna_list_get_count(list);
	
	if(sz > 0)
	{
	    anna_entry_t *callback_param[] = 
		{
		    anna_from_obj(list),
		    anna_from_obj(body),
		    anna_from_int(1),
		    anna_from_obj(res)
		}
	    ;
	    
	    anna_entry_t *o_param[] =
		{
		    anna_from_int(0),
		    anna_list_get(list, 0)
		}
	    ;
	    
	    anna_vm_callback_native(
		stack,
		anna_list_map_callback, 4, callback_param,
		body, 2, o_param
		);
	}
	else
	{
	    anna_context_push_object(stack, res);
	}
    }
}

static void anna_list_filter_callback(anna_context_t *stack)
{
    anna_entry_t *value = anna_context_pop_entry(stack);

    anna_entry_t **param = stack->top - 4;
    anna_object_t *list = anna_as_obj_fast(	param[0]);
    anna_object_t *body = anna_as_obj_fast(param[1]);
    int idx = anna_as_int(param[2]);
    anna_object_t *res = anna_as_obj_fast(param[3]);
    size_t sz = anna_list_get_count(list);

    if(!anna_entry_null(value))
    {
	anna_list_add(res, anna_list_get(list, idx-1));
    }
    
    if((sz > idx) && !(stack->frame->flags & ANNA_ACTIVATION_FRAME_BREAK))
    {
	anna_entry_t *o_param[] =
	    {
		param[2],
		anna_list_get(list, idx)
	    }
	;
	
	param[2] = anna_from_int(idx+1);
	anna_vm_callback_reset(stack, body, 2, o_param);
    }
    else
    {
	anna_context_drop(stack, 5);
	anna_context_push_object(stack, res);
    }    
}

static void anna_list_filter(anna_context_t *stack)
{
    anna_object_t *body = anna_context_pop_object(stack);
    anna_object_t *list = anna_context_pop_object(stack);
    anna_object_t *res = anna_list_is_mutable(list)?
	anna_list_create_mutable(anna_list_get_specialization(list)):
	anna_list_create_imutable(anna_list_get_specialization(list));
    anna_context_pop_entry(stack);
    
    size_t sz = anna_list_get_count(list);
    
    if(sz > 0)
    {
	anna_entry_t *callback_param[] = 
	    {
		anna_from_obj(list),
		anna_from_obj(body),
		anna_from_int(1),
		anna_from_obj(res)
	    }
	;
	
	anna_entry_t *o_param[] =
	    {
		anna_from_int(0),
		anna_list_get(list, 0)
	    }
	;
	
	anna_vm_callback_native(
	    stack,
	    anna_list_filter_callback, 4, callback_param,
	    body, 2, o_param
	    );
    }
    else
    {
	anna_context_push_object(stack, res);
    }
}

static void anna_list_find_callback(anna_context_t *stack)
{
    anna_entry_t *value = anna_context_pop_entry(stack);

    anna_entry_t **param = stack->top - 3;
    anna_object_t *list = anna_as_obj_fast(param[0]);
    anna_object_t *body = anna_as_obj_fast(param[1]);
    int idx = anna_as_int(param[2]);
    size_t sz = anna_list_get_count(list);

    if(!anna_entry_null(value))
    {
	anna_context_drop(stack, 4);
	anna_context_push_entry(stack, anna_list_get(list, idx-1));
    }
    else if( (sz > idx) && !(stack->frame->flags & ANNA_ACTIVATION_FRAME_BREAK))
    {
	anna_entry_t *o_param[] =
	    {
		param[2],
		anna_list_get(list, idx)
	    }
	;
	
	param[2] = anna_from_int(idx+1);
	anna_vm_callback_reset(stack, body, 2, o_param);
    }
    else
    {
	anna_context_drop(stack, 4);
	anna_context_push_object(stack, null_object);
    }
}

static void anna_list_find(anna_context_t *stack)
{
    anna_object_t *body = anna_context_pop_object(stack);
    anna_object_t *list = anna_context_pop_object(stack);
    anna_context_pop_entry(stack);
    
    size_t sz = anna_list_get_count(list);
    
    if(sz > 0)
    {
	anna_entry_t *callback_param[] = 
	    {
		anna_from_obj(list),
		anna_from_obj(body),
		anna_from_int(1),
	    }
	;
	
	anna_entry_t *o_param[] =
	    {
		anna_from_int(0),
		anna_list_get(list, 0)
	    }
	;
	
	anna_vm_callback_native(
	    stack,
	    anna_list_find_callback, 3, callback_param,
	    body, 2, o_param
	    );
    }
    else
    {
	anna_context_push_object(stack, null_object);
    }
}


ANNA_VM_NATIVE(anna_list_init, 2)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_object_t *that = anna_as_obj_fast(param[1]);
    (*anna_entry_get_addr(this,ANNA_MID_LIST_PAYLOAD))=0;
    (*(size_t *)anna_entry_get_addr(this,ANNA_MID_LIST_CAPACITY)) = 0;    
    (*(size_t *)anna_entry_get_addr(this,ANNA_MID_LIST_SIZE)) = 0;
    
    this->flags |= ANNA_OBJECT_LIST;
    
    size_t sz = anna_list_get_count(that);
    anna_entry_t **src = anna_list_get_payload(that);
    
    anna_list_set_count(this, sz);
    anna_entry_t **dest = anna_list_get_payload(this);
    memcpy(dest, src, sizeof(anna_object_t *)*sz);
    
    return param[0];
}

static void anna_list_del(anna_object_t *victim)
{
    free(anna_entry_get(victim,ANNA_MID_LIST_PAYLOAD));
    (*(size_t *)anna_entry_get_addr(victim,ANNA_MID_LIST_CAPACITY)) = 0;    
    (*(size_t *)anna_entry_get_addr(victim,ANNA_MID_LIST_SIZE)) = 0;
}

ANNA_VM_NATIVE(anna_list_push, 2)
{
    anna_list_set(
	anna_as_obj_fast(param[0]), 
	(*(size_t *)anna_entry_get_addr(anna_as_obj_fast(param[0]),ANNA_MID_LIST_SIZE)),
	param[1]);
    return param[0];
}

ANNA_VM_NATIVE(anna_list_pop, 1)
{
    
    size_t *sz = (size_t *)anna_entry_get_addr(anna_as_obj_fast(param[0]),ANNA_MID_LIST_SIZE);
    if(!*sz)
	return null_entry;
    (*sz)--;
    return (*(anna_entry_t ***)anna_entry_get_addr(anna_as_obj_fast(param[0]),ANNA_MID_LIST_PAYLOAD))[*sz];
}

static void anna_list_in_callback(anna_context_t *stack)
{
    anna_entry_t *ret = anna_context_pop_entry(stack);
//    wprintf(L"Wee, in callback value: %ls\n", ret==null_object?L"null":L"not null");
    anna_entry_t **param = stack->top - 3;
    anna_object_t *list = anna_as_obj_fast(param[0]);
    anna_object_t *value = anna_as_obj_fast(param[1]);
    int idx = anna_as_int(param[2]);
    size_t sz = anna_list_get_count(list);
    
    if(!anna_entry_null(ret))
    {
	anna_context_drop(stack, 4);
	anna_context_push_entry(stack, anna_from_int(idx-1));
    }
    else if(sz > idx)
    {
	anna_entry_t *o_param[] =
	    {
		anna_from_obj(value),
		anna_list_get(list, idx)
	    }
	;
	
	anna_object_t *fun_object = anna_as_obj_fast(anna_entry_get_static(value->type, ANNA_MID_EQ));
	param[2] = anna_from_int(idx+1);
	anna_vm_callback_reset(stack, fun_object, 2, o_param);
    }
    else
    {
	anna_context_drop(stack, 4);
	anna_context_push_object(stack, null_object);
    }
}

static void anna_list_in(anna_context_t *stack)
{
    anna_object_t *value = anna_context_pop_object(stack);
    anna_object_t *list = anna_context_pop_object(stack);
    anna_context_pop_entry(stack);
    
    size_t sz = anna_list_get_count(list);
    
    if(sz > 0)
    {
	anna_entry_t *callback_param[] = 
	    {
		anna_from_obj(list),
		anna_from_obj(value),
		anna_from_int(1),
	    }
	;
	
	anna_entry_t *o_param[] =
	    {
		anna_from_obj(value),
		anna_list_get(list, 0)
	    }
	;
	
	anna_object_t *fun_object = anna_as_obj_fast(anna_entry_get_static(value->type, ANNA_MID_EQ));
	anna_vm_callback_native(
	    stack,
	    anna_list_in_callback, 3, callback_param,
	    fun_object, 2, o_param
	    );
    }
    else
    {
	anna_context_push_object(stack, null_object);
    }
}

ANNA_VM_NATIVE(anna_list_i_get_range, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[1]);
    anna_object_t *list = anna_as_obj(param[0]);
    anna_object_t *range = anna_as_obj(param[1]);
    int from = anna_range_get_from(range);
    int step = anna_range_get_step(range);
    int to = anna_range_get_to(range);
    int i;

    if(anna_range_get_open(range))
    {
	to = step>0?anna_list_get_count(list):-1;
    }
    
    anna_object_t *res = anna_list_is_mutable(list)?
	anna_list_create_mutable(anna_list_get_specialization(list)):
	anna_list_create_imutable(anna_list_get_specialization(list));

    for(i=from;(step>0)? i<to : i>to; i+=step)
    {
	anna_list_add(
	    res, 
	    anna_list_get(
		list,
		i));
    }
    
    return anna_from_obj(res);
    
}

ANNA_VM_NATIVE(anna_list_i_set_range, 3)
{
    ANNA_ENTRY_NULL_CHECK(param[1]);
    
    anna_object_t *repl;
    anna_object_t *list = anna_as_obj(param[0]);
    anna_object_t *range = anna_as_obj(param[1]);
    
    if(anna_entry_null(param[2]))
	repl = anna_list_create_mutable(object_type);
    else
	repl = anna_as_obj(param[2]);
    
    int from = anna_range_get_from(range);
    int step = anna_range_get_step(range);
    int to = anna_range_get_to(range);
    int count = anna_range_get_count(range);
    int i;

    if(anna_range_get_open(anna_as_obj_fast(param[1])))
    {
	to = step>0?anna_list_get_count(list):-1;
    }
    
    count = (1+(to-from-sign(step))/step);
    
    int count2 = anna_list_get_count(repl);

    if(count != count2)
    {
	if(step != 1)
	{
	    return null_entry;
	}

	int old_size = anna_list_get_count(list);

	/* If we're assigning past the end of the array, just silently
	 * take the whole array and go on */
	count = mini(count, old_size - from);	
	int new_size = old_size - count + count2;
	anna_entry_t **arr;
	if(to > new_size)
	{
	    anna_list_set_capacity(list, to);
	    for(i=old_size; i<new_size; i++)
	    {
		anna_list_set(
		    list, i, null_entry);		
	    }
	    *(size_t *)anna_entry_get_addr(list,ANNA_MID_LIST_SIZE) = new_size;
	    arr = anna_list_get_payload(list);
	}
	else
	{
	    if(new_size > anna_list_get_capacity(list))
	    {
		anna_list_set_capacity(list, new_size);
	    }
	    
	    *(size_t *)anna_entry_get_addr(list,ANNA_MID_LIST_SIZE) = new_size;
	    arr = anna_list_get_payload(list);
	    memmove(&arr[from+count2], &arr[from+count], sizeof(anna_object_t *)*abs(old_size - from - count ));
	}
	
	/* Set new size - don't call anna_list_set_count, since that might truncate the list if we're shrinking */

	/* Move the old data */

	/* Copy in the new data */
	for(i=0;i<count2;i++)
	{
	    arr[from+i] = 
		anna_list_get(
		    repl,
		    i);
	}
    }
    else
    {
	for(i=0;i<count;i++)
	{
	    anna_list_set(
		list, from + step*i, 
		anna_list_get(
		    repl,
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
	sizeof(anna_entry_t *) * anna_list_get_count(this));
    
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
	sizeof(anna_entry_t *) * anna_list_get_count(this));
    
    return anna_from_obj(that);
}

static void anna_list_type_create_internal(
    anna_type_t *type, 
    anna_type_t *spec,
    anna_type_t *imutable_type, 
    anna_type_t *mutable_type, 
    anna_type_t *intersection_type,
    int mutable)
{
    mid_t mmid;
    anna_function_t *fun;

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
	1,
	null_type);
    (*(anna_type_t **)anna_entry_get_addr_static(type,ANNA_MID_LIST_SPECIALIZATION)) = spec;
    
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

    anna_type_t *l_argv[] = 
	{
	    type,
	    type
	}
    ;
    
    wchar_t *l_argn[]=
	{
	    L"this", L"value"
	}
    ;

    anna_member_create_native_method(
	type, anna_mid_get(L"__init__"),
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

    mmid = anna_member_create_native_method(
	type,
	anna_mid_get(L"__get__Int__"), 0,
	&anna_list_get_int, spec, 2,
	i_argv, i_argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(type, mmid)));
    anna_function_alias_add(fun, L"__get__");

    anna_member_create_native_property(
	type, anna_mid_get(L"count"), int_type,
	&anna_list_get_count_method,
	mutable ? &anna_list_set_count_method : 0,
	L"The number of elements in this list.");

    anna_member_create_native_property(
	type,
	anna_mid_get(L"first"),
	spec,
	&anna_list_get_first, 0,
	L"The first element of this list.");

    anna_member_create_native_property(
	type,
	anna_mid_get(L"last"),
	spec,
	&anna_list_get_last,
	0,
	L"The last element of this list");

    anna_member_create_native_method(
	type, anna_mid_get(L"__appendAssign__"),
	0, &anna_list_append, type, 2, l_argv,
	l_argn, 0, 0);

    anna_type_t *fun_type = anna_function_type_each_create(
	L"!ListIterFunction", int_type, spec);

    anna_type_t *e_argv[] = 
	{
	    type,
	    fun_type
	}
    ;
    
    wchar_t *e_argn[]=
	{
	    L"this", L"block"
	}
    ;    
    
    anna_member_create_native_method(
	type, anna_mid_get(L"__each__"), 0,
	&anna_list_each, type, 2, e_argv, e_argn, 0, 0);
    
    anna_member_create_native_method(
	type,
	anna_mid_get(L"__filter__"),
	0,
	&anna_list_filter,
	mutable?mutable_type:imutable_type,
	2,
	e_argv,
	e_argn, 0,
	L"Execute the specified function once for each element in the list, and return a new list containing all the elements for which the function retuned non-null");

    anna_member_create_native_method(
	type,
	anna_mid_get(L"__find__"),
	0,
	&anna_list_find,
	spec,
	2,
	e_argv,
	e_argn, 0, 0);
    
    anna_member_create_native_method(
	type,
	anna_mid_get(L"__map__"),
	0,
	&anna_list_map,
	mutable?mutable_list_type:imutable_list_type,
	2,
	e_argv,
	e_argn, 0, 0);
    
    anna_member_create_native_method(
	type,
	anna_mid_get(L"__in__"),
	0,
	&anna_list_in,
	int_type,
	2,
	a_argv,
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

    mmid = anna_member_create_native_method(
	type,
	anna_mid_get(L"__get__Range__"), 0,
	&anna_list_i_get_range, type, 2,
	range_argv, range_argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(type, mmid)));
    anna_function_alias_add(fun, L"__get__");

    anna_list_add_all_extra_methods(type);

    if(mutable)
    {
	
	mmid = anna_member_create_native_method(
	    type,
	    anna_mid_get(L"__set__Int__"), 0,
	    &anna_list_set_int, spec, 3,
	    i_argv, i_argn, 0, 0);
	fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(type, mmid)));	anna_function_alias_add(fun, L"__set__");

	anna_member_create_native_method(
	    type, anna_mid_get(L"push"),
	    0, &anna_list_push,
	    type,
	    2,
	    a_argv,
	    a_argn, 0, 
	    L"Adds the specified element to the end of the list. Returns the mutated list.");
	
	anna_member_create_native_method(
	    type, anna_mid_get(L"pop"), 0,
	    &anna_list_pop, spec, 1, a_argv, a_argn, 0, 
	    L"Removes the last element from the list and returns it. Returns null if the list is already empty.");
	
	mmid = anna_member_create_native_method(
	    type,
	    anna_mid_get(L"__set__Range__"), 0,
	    &anna_list_i_set_range, type, 3,
	    range_argv, range_argn, 0, 0);
	fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(type, mmid)));	anna_function_alias_add(fun, L"__set__");

    }

    anna_member_create_native_property(
	type, anna_mid_get(L"freeze"),
	imutable_type, mutable ? &anna_list_i_copy_imutable : &anna_util_noop,
	0,
	L"An imutable copy of this List, or the List itself if it is already imutable.");
    
    anna_member_create_native_property(
	type, anna_mid_get(L"thaw"),
	mutable_type, mutable ? &anna_util_noop : &anna_list_i_copy_mutable, 0,
	L"A mutable copy of this List, or the List itself if it is already mutable.");

    anna_type_close(type);
    
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
    return res;
}

void anna_list_type_create()
{
    anna_list_internal_init();

    anna_list_type_insert(
	object_type, mutable_list_type, imutable_list_type, any_list_type);
    
    anna_list_type_create_internal(
	mutable_list_type, object_type, 
	imutable_list_type, mutable_list_type, any_list_type, 1);
    anna_list_type_create_internal(
	imutable_list_type, object_type, 
	imutable_list_type, mutable_list_type, any_list_type, 0);
    anna_type_intersect_into(
	any_list_type, mutable_list_type, imutable_list_type);    

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
	anna_type_t *mutable = anna_type_native_create(sb_content(&sb), 0);

	sb_truncate(&sb, 0);
	sb_printf(&sb, L"ImutableList«%ls»", subtype->name);
	anna_type_t *imutable = anna_type_native_create(
	    sb_content(&sb), 0);
	
	sb_truncate(&sb, 0);
	sb_printf(&sb, L"List«%ls»", subtype->name);
	anna_type_t *any = anna_type_native_create(
	    sb_content(&sb), 0);
	
	sb_destroy(&sb);

	res = anna_list_type_insert(subtype, mutable, imutable, any);
	
	anna_list_type_create_internal(
	    mutable, subtype, imutable, mutable, any, 1);
	anna_list_type_create_internal(
	    imutable, subtype, imutable, mutable, any, 0);
	anna_type_intersect_into(any, mutable, imutable);
	
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

static void add_list_mark_method(void *key, void *value)
{
    anna_type_t **list = (anna_type_t **)value;
    anna_alloc_mark_type(list[0]);
    anna_alloc_mark_type(list[1]);
    anna_alloc_mark_type(list[2]);
}

void anna_list_mark_static(void)
{
    hash_foreach(&anna_list_specialization, &add_list_mark_method);
}

