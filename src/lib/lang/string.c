//ROOT: src/lib/lang/lang.c
#include "src/lib/lang/string_internal.c"

static int anna_string_seed;

static inline anna_string_t *as_unwrap(anna_object_t *obj)
{
    return (anna_string_t *)anna_entry_get_addr_fast(obj,ANNA_MID_STRING_PAYLOAD);
}

void anna_string_print(anna_object_t *obj)
{
    anna_string_t *str = as_unwrap(obj);
    asi_print_regular(str);
}

void anna_string_append(anna_object_t *this, anna_object_t *str)
{
    asi_append(as_unwrap(this), as_unwrap(str), 0, asi_get_count(as_unwrap(str)));
}

void anna_string_append_cstring(anna_object_t *this, wchar_t *str, size_t len)
{
    asi_append_cstring(as_unwrap(this), str, len);
}


anna_object_t *anna_string_create(size_t sz, wchar_t *data)
{
    anna_object_t *obj= anna_object_create_raw(imutable_string_type);
    // anna_message(L"Create new string \"%.*ls\" at %d\n", sz, data, obj);
    
    asi_init_from_ptr(as_unwrap(obj),  data, sz);
    return obj;
}

anna_object_t *anna_string_create_narrow(size_t sz, char *data)
{
    wchar_t *wdata = malloc(sizeof(wchar_t)*(sz+1));
    size_t wsz;
    
    if((wsz = mbstowcs(wdata, data, sz)) == (size_t)-1)
    {
	free(wdata);
	return null_object;
    }
    
    anna_object_t *obj= anna_object_create_raw(imutable_string_type);
    // anna_message(L"Create new string \"%.*ls\" at %d\n", sz, data, obj);
    
    asi_init_from_ptr(as_unwrap(obj),  wdata, wsz);
    return obj;
}

anna_object_t *anna_string_copy(anna_object_t *orig)
{
    anna_object_t *obj= anna_object_create_raw(imutable_string_type);
    //  anna_message(L"Create new string \"%.*ls\" at %d\n", sz, data, obj);
    
    asi_init(as_unwrap(obj));
    anna_string_t *o = as_unwrap(orig);
    asi_append(as_unwrap(obj), o, 0, asi_get_count(o));
    return obj;
}

static anna_object_t *anna_mutable_string_copy(anna_object_t *orig)
{
    anna_object_t *obj= anna_object_create_raw(mutable_string_type);
    //  anna_message(L"Create new string \"%.*ls\" at %d\n", sz, data, obj);
    
    asi_init(as_unwrap(obj));
    anna_string_t *o = as_unwrap(orig);
    asi_append(as_unwrap(obj), o, 0, asi_get_count(o));
    return obj;
}

size_t anna_string_get_count(anna_object_t *this)
{
    return asi_get_count(as_unwrap(this));
}

wchar_t *anna_string_payload(anna_object_t *obj)
{
//    anna_message(L"Get payload from string at %d\n", obj);
    anna_string_t *str = as_unwrap(obj);
    return asi_cstring(str);
}

char *anna_string_payload_narrow(anna_object_t *obj)
{
//    anna_message(L"Get payload from string at %d\n", obj);
    anna_string_t *str = as_unwrap(obj);
    return asi_cstring_narrow(str);
}

static ssize_t anna_string_idx_wrap(anna_object_t *str, ssize_t idx)
{
    if(idx < 0)
    {
	return (ssize_t)anna_string_get_count(str) + idx;
    }
    return idx;
}

ANNA_VM_NATIVE(anna_string_init, 1)
{
    anna_object_t *obj= anna_as_obj(param[0]);
    asi_init(as_unwrap(obj));
    return param[0];
}

ANNA_VM_NATIVE(anna_string_i_set_int, 3)
{
    ANNA_ENTRY_NULL_CHECK(param[1]);
    ANNA_ENTRY_NULL_CHECK(param[2]);
    wchar_t ch = anna_as_char(param[2]);
    ssize_t idx = anna_string_idx_wrap(anna_as_obj(param[0]), anna_as_int(param[1]));
    if(likely(idx >= 0))
    {
	asi_set_char(as_unwrap(anna_as_obj(param[0])), idx, ch);
    }
    return param[2];
}

ANNA_VM_NATIVE(anna_string_i_get_int, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[1]);
    ssize_t idx = anna_string_idx_wrap(anna_as_obj(param[0]), anna_as_int(param[1]));
    if(!(idx < 0 || idx >= anna_string_get_count(anna_as_obj(param[0]))))
    {
	return anna_from_char(asi_get_char(as_unwrap(anna_as_obj(param[0])), idx));
    }
    return null_entry;
}

ANNA_VM_NATIVE(anna_string_i_get_range, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[1]);
    
    anna_object_t *this = anna_as_obj_fast(param[0]);
    ssize_t from = anna_string_idx_wrap(anna_as_obj_fast(param[0]), anna_range_get_from(anna_as_obj_fast(param[1])));
    ssize_t to = anna_string_idx_wrap(anna_as_obj_fast(param[0]), anna_range_get_to(anna_as_obj_fast(param[1])));
    ssize_t step = anna_range_get_step(anna_as_obj_fast(param[1]));
    
    if(anna_range_get_open(anna_as_obj_fast(param[1])))
    {
	to = step>0?anna_string_get_count(this):-1;
    }

    if((to > from) != (step > 0))
    {
	step = -step;
    }

    anna_object_t *res = anna_object_create(imutable_string_type);
    asi_init(as_unwrap(res));
    if(step == 1)
    {
	asi_append(as_unwrap(res), as_unwrap(anna_as_obj_fast(param[0])), from, to-from);
    }
    else
    {
	int i;
	
	for(i=from;(step>0)? i<to : i>to; i+=step)
	{
	    wchar_t ch = asi_get_char(as_unwrap(this), i);
	    asi_append_cstring(as_unwrap(res), &ch, 1);
	}
	
    }
    
    return anna_from_obj(res);
}



ANNA_VM_NATIVE(anna_string_i_mul, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    ANNA_ENTRY_NULL_CHECK(param[1]);
    
    anna_object_t *this = anna_as_obj_fast(param[0]);
    int times = anna_as_int(param[1]);
    
    anna_object_t *res = anna_object_create(this->type);
    asi_init(as_unwrap(res));
    int i;
    size_t length = asi_get_count(as_unwrap(this));
    for(i=0; i<times; i++)
    {
	asi_append(as_unwrap(res), as_unwrap(this), 0, length);
    }
    return anna_from_obj(res);
}



ANNA_VM_NATIVE(anna_string_i_set_range, 3)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    ANNA_ENTRY_NULL_CHECK(param[1]);
	
    anna_object_t *this_obj = anna_as_obj(param[0]);
    anna_object_t *range = anna_as_obj(param[1]);
    anna_object_t *replacement_obj;
    
    if(anna_entry_null(param[2]))
    {
	replacement_obj = anna_string_create(0, L"");
    }
    else
    {
	replacement_obj = anna_as_obj(param[2]);
    }
    
    ssize_t from_orig = anna_range_get_from(range);
    ssize_t to_orig = anna_range_get_to(range);
    ssize_t from = anna_string_idx_wrap(this_obj, from_orig);
    ssize_t to = anna_string_idx_wrap(this_obj, to_orig);
    ssize_t step = anna_range_get_step(range);
    ssize_t count;
    
    if(from < 0)
    {
	return null_entry;
    }    
    
    anna_string_t *this = as_unwrap(this_obj);
    size_t count_this = asi_get_count(this);
    anna_string_t *replacement = as_unwrap(replacement_obj);
    size_t count_replacement = asi_get_count(replacement);

    if(anna_range_get_open(anna_as_obj_fast(param[1])))
    {
	to = step>0?count_this:-1;
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
	
    ssize_t i;
	
    if(count_replacement == 0)
    {
	/*
	  Erase mode.

	  The replacement string is either empty or null. In this
	  case, we remove all the elements in the specified slice from
	  the string.
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
	
	int old_size = count_this;
	int new_size = maxi(0, old_size - count);

	if(to!=old_size || step!=1)
	{
	    int out = from;
	    int in;
	    for(in=from; in < old_size; in++)
	    {
		if((in >= to) || (in-from)%step != 0)
		{
		    asi_set_char(this, out++, asi_get_char(this, in));
		}
	    }
	    asi_truncate(this, out);
	}
	else
	{
	    asi_truncate(this, new_size);
	}
	
    }
    else if(step==1)
    {
	/*
	  Short cut for the common case
	*/
	asi_replace(
	    this,
	    replacement,
	    from,
	    to-from,
	    0,
	    count_replacement);
    }    
    else if(count == count_replacement)
    {
	for(i=0; i<count;i++)
	{
	    asi_set_char(
		this,
		from + step*i,
		asi_get_char(
		    replacement,
		    i));
	}   
    }
    else
    {
	if(count == 0)
	{
	    step=1;
	}
	
	if(abs(step) != 1)
	{
	    return null_entry;
	}
	
	int old_size = count_this;

	count = mini(count, old_size - from);
	int new_size = old_size - count + count_replacement;

	if(new_size > old_size)
	{
	    asi_set_char(this, new_size-1, 0);
	}
	else
	{
	    asi_truncate(this, new_size);
	}
	
	/* Move the old data */
	FIXME("The internal string implementation isn't treated as an opaque datatype here.");
	
	memmove(
	    &this->str[mini(from,to)+count_replacement], 
	    &this->str[mini(from,to)+count], 
	    sizeof(wchar_t)*abs(old_size - mini(from,to) - count ));

	/* Copy in the new data */
	int offset = (step > 0) ? (from) : (from+count_replacement-count);
	for(i=0;i<count_replacement;i++)
	{
	    asi_set_char(
		this, offset+step*i,
		asi_get_char(
		    replacement,
		    i));
	}	
    }
    
    return param[0];
}

ANNA_VM_NATIVE(anna_string_i_copy, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    return anna_from_obj(anna_string_copy(this));
}

ANNA_VM_NATIVE(anna_mutable_string_i_copy, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    return anna_from_obj(anna_mutable_string_copy(this));
}

ANNA_VM_NATIVE(anna_string_i_get_count, 1)
{
    return anna_from_int(asi_get_count(as_unwrap(anna_as_obj_fast(param[0]))));
}

ANNA_VM_NATIVE(anna_string_i_get_lower, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_object_t *res =  this->type == mutable_string_type ? 
	anna_mutable_string_copy(this) : 
	anna_string_copy(this);
    anna_string_t *str = as_unwrap(res);
    
    for(size_t i=0; i < asi_get_count(str); i++) 
    {
	wchar_t ch = asi_get_char(str, i);
	asi_set_char(str, i, towlower(ch));
    }
    return anna_from_obj(res);
}

ANNA_VM_NATIVE(anna_string_i_get_upper, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_object_t *res =  this->type == mutable_string_type ? 
	anna_mutable_string_copy(this) : 
	anna_string_copy(this);
    anna_string_t *str = as_unwrap(res);
    
    for(size_t i=0; i < asi_get_count(str); i++) 
    {
	wchar_t ch = asi_get_char(str, i);
	asi_set_char(str, i, towupper(ch));
    }
    return anna_from_obj(res);
}

ANNA_VM_NATIVE(anna_string_i_set_count, 2)
{
    anna_object_t *this = anna_as_obj(param[0]);
    if(!anna_entry_null(param[1]))
    {
	int sz = anna_as_int(param[1]);
	asi_truncate(as_unwrap(this), sz);
    }
    return param[1];
}

static void anna_string_join_callback(anna_context_t *context)
{    
    anna_object_t *str_obj2 = anna_context_pop_object(context);
    anna_object_t *str_obj = anna_context_pop_object(context);
    anna_object_t *res = null_object;
    if((str_obj2->type == mutable_string_type) || (str_obj2->type == imutable_string_type))
    {
	anna_string_t *str = as_unwrap(str_obj);
	anna_string_t *str2 = as_unwrap(str_obj2);
	res = anna_object_create(imutable_string_type);
	
	asi_init(as_unwrap(res));
	asi_append(as_unwrap(res), str, 0, asi_get_count(str));
	asi_append(as_unwrap(res), str2, 0, asi_get_count(str2));
    }
    anna_context_pop_entry(context);
    anna_context_push_object(context, res);
}

static void anna_string_i_join(anna_context_t *context)
{
    anna_entry_t e = anna_context_pop_entry(context);
    anna_object_t *this = anna_context_pop_object(context);
    anna_context_pop_entry(context);
    
    if(anna_entry_null(e))
    {
	anna_context_push_object(context, null_object);
    }
    else if(anna_is_int_small(e))
    {	
	anna_object_t *res = anna_object_create(this->type);
	anna_string_t *unwrapped = as_unwrap(res);
	wchar_t is[32];
	int is_len = swprintf(is, 32, L"%d", anna_as_int(e));
	asi_init(as_unwrap(res));
	asi_ensure_capacity(unwrapped, asi_get_count(as_unwrap(this)) + is_len);
	asi_append(unwrapped, as_unwrap(this), 0, asi_get_count(as_unwrap(this)));
	asi_append_cstring(unwrapped, is, is_len);
	
	anna_context_push_object(context, res);
    }
    else
    {
	anna_object_t *o = anna_as_obj(e);
	if((o->type == mutable_string_type) ||
	   (o->type == imutable_string_type))
	{
	    anna_object_t *res = anna_object_create(this->type);
	    asi_init(as_unwrap(res));
	    asi_append(as_unwrap(res), as_unwrap(this), 0, asi_get_count(as_unwrap(this)));
	    asi_append(as_unwrap(res), as_unwrap(o), 0, asi_get_count(as_unwrap(o)));
	    
	    anna_context_push_object(context, res);
	}
	else
	{
	    anna_object_t *fun_object = anna_as_obj_fast(anna_entry_get_static(o->type, ANNA_MID_TO_STRING));
	    anna_entry_t callback_param[] = 
		{
		    anna_from_obj(this),
		}
	    ;
	    
	    anna_entry_t o_param[] =
		{
		    anna_from_obj(o)
		}
	    ;
	    
	    anna_vm_callback_native(
		context,
		anna_string_join_callback, 1, callback_param,
		fun_object, 1, o_param
		);
	}
    }
}

static void anna_string_convert_callback(anna_context_t *context)
{
    anna_object_t *str_obj = anna_context_pop_object(context);
    anna_object_t *res = null_object;
    if((str_obj->type == mutable_string_type) || (str_obj->type == imutable_string_type))
    {
	res = str_obj;
    }
    anna_context_pop_entry(context);
    anna_context_push_object(context, res);
}

static void anna_string_convert(anna_context_t *context)
{
    anna_entry_t e = anna_context_pop_entry(context);
    anna_context_pop_entry(context);
    
    if(anna_entry_null(e))
    {
	anna_object_t *res = anna_object_create(imutable_string_type);
	asi_init(as_unwrap(res));
	asi_append_cstring(as_unwrap(res), L"?", 1);	
	anna_context_push_object(context, res);
    }
    else if(anna_is_int_small(e))
    {
	anna_object_t *res = anna_object_create(imutable_string_type);
	wchar_t is[32];
	swprintf(is, 32, L"%d", anna_as_int(e));
	asi_init(as_unwrap(res));
	asi_append_cstring(as_unwrap(res), is, wcslen(is));	
	anna_context_push_object(context, res);
    }
    else
    {
	anna_object_t *o = anna_as_obj(e);
	anna_entry_t *fun_ptr = anna_entry_get_addr_static(o->type, ANNA_MID_TO_STRING);
	int ok = 0;

	if((o->type == mutable_string_type) ||
	   (o->type == imutable_string_type))
	{
	    anna_context_push_object(context, o);
	}
	else
	{    
	    if(fun_ptr)
	    {
		anna_object_t *fun_object = anna_as_obj(*fun_ptr);
		
		if(fun_object)
		{
		    anna_function_t *fun = anna_function_unwrap(fun_object);
		    if(fun && fun->input_count == 1 && anna_abides(o->type, fun->input_type[0]))
		    {
			ok = 1;
			anna_entry_t o_param[] = { anna_from_obj(o) };
			anna_vm_callback_native(
			    context,
			    anna_string_convert_callback, 0, 0,
			    fun_object, 1, o_param
			    );
		    }
		}
	    }
	    
	    if(!ok)
	    {
		anna_object_t *res = anna_object_create(imutable_string_type);
		asi_init(as_unwrap(res));
		wchar_t *msg = L"<Object of type ";
		asi_append_cstring(as_unwrap(res), msg, wcslen(msg));	
		asi_append_cstring(as_unwrap(res), o->type->name, wcslen(o->type->name));	
		asi_append_cstring(as_unwrap(res), L">", 1);	
		anna_context_push_object(context, res);	
	    }
	}
	
    }
}

static void anna_string_ljoin_callback(anna_context_t *context)
{    
    anna_object_t *value = anna_context_pop_object(context);
    anna_entry_t *param = context->top - 4;
    anna_object_t *joint = anna_as_obj_fast(param[0]);
    anna_object_t *list = anna_as_obj_fast(param[1]);
    int idx = anna_as_int(param[2]);
    anna_object_t *res = anna_as_obj_fast(param[3]);
    size_t sz = anna_list_get_count(list);

    if(value == null_object) 
    {
	anna_context_drop(context, 5);
	anna_context_push_object(context, null_object);
	return;
    }

    if(idx>1){
	asi_append(as_unwrap(res), as_unwrap(joint), 0, asi_get_count(as_unwrap(joint)));
    }
    asi_append(as_unwrap(res), as_unwrap(value), 0, asi_get_count(as_unwrap(value)));
    
    if(sz > idx)
    {
	param[2] = anna_from_int(idx+1);
	anna_object_t *o = anna_as_obj(anna_list_get(list, idx));
	anna_member_t *tos_mem = anna_member_get(o->type, ANNA_MID_TO_STRING);
	anna_object_t *meth = anna_as_obj_fast(o->type->static_member[tos_mem->offset]);
	anna_vm_callback_reset(context, meth, 1, (anna_entry_t *)&o);
    }
    else
    {
	anna_context_drop(context, 5);
	anna_context_push_object(context, res);
    }
}

static void anna_string_i_ljoin(anna_context_t *context)
{
    anna_object_t *list = anna_context_pop_object(context);
    anna_object_t *joint = anna_context_pop_object(context);
    anna_context_pop_entry(context);

    if(list == null_object)
    {
	anna_context_push_object(context, null_object);
    }
    else
    {
	size_t sz = anna_list_get_count(list);
	
	if(sz > 0)
	{
	    anna_entry_t callback_param[] = 
		{
		    anna_from_obj(joint),
		    anna_from_obj(list),
		    anna_from_int(1),
		    anna_from_obj(anna_string_create(0,0))
		}
	    ;
	    
	    anna_object_t *o = anna_as_obj(anna_list_get(list, 0));
	    anna_member_t *tos_mem = anna_member_get(o->type, ANNA_MID_TO_STRING);
	    anna_object_t *meth = anna_as_obj_fast(o->type->static_member[tos_mem->offset]);
	    
	    anna_vm_callback_native(
		context,
		anna_string_ljoin_callback, 4, callback_param,
		meth, 1, (anna_entry_t *)&o
		);
	}
	else
	{
	    anna_context_push_object(context, anna_string_create(0,0));
	}
    }
}

static void anna_string_append_callback(anna_context_t *context)
{
    anna_object_t *value = anna_context_pop_object(context);
    anna_entry_t *param = context->top - 1;
    anna_object_t *this = anna_as_obj(param[0]);
    anna_context_drop(context, 2);

    if(value == null_object) 
    {
	anna_context_push_object(context, null_object);
    }
    else
    {
	asi_append(as_unwrap(this), as_unwrap(value), 0, asi_get_count(as_unwrap(value)));
	anna_context_push_object(context, this);        
    }
}

static void anna_string_i_append(anna_context_t *context)
{
    anna_object_t *obj = anna_context_pop_object(context);
    anna_object_t *this = anna_context_pop_object(context);
    anna_context_pop_entry(context);
    
    if(obj!=null_object)
    {
	if((obj->type == mutable_string_type) ||
	   (obj->type == imutable_string_type))
	{
	    asi_append(as_unwrap(this), as_unwrap(obj), 0, asi_get_count(as_unwrap(obj)));
	    anna_context_push_object(context, this);
	}
	else
	{
	    anna_entry_t callback_param[] = 
		{
		    anna_from_obj(this)
		}
	    ;
	    
	    anna_member_t *tos_mem = anna_member_get(obj->type, ANNA_MID_TO_STRING);
	    anna_object_t *meth = anna_as_obj_fast(obj->type->static_member[tos_mem->offset]);
	    anna_vm_callback_native(
		context,
		anna_string_append_callback, 1, callback_param,
		meth, 1, (anna_entry_t *)&obj
		);
	}
    }
    else{
	anna_context_push_object(context, this);
    }
}

static void anna_string_del(anna_object_t *victim)
{
//    debug(99, L"HEJ BABERIBA\n");
    asi_destroy(as_unwrap(victim));
}

ANNA_VM_NATIVE(anna_string_to_string, 1)
{
    return param[0];
}

int anna_string_cmp(anna_object_t *this, anna_object_t *that)
{
    anna_string_t *str1 = as_unwrap(this);
    anna_string_t *str2 = as_unwrap(that);
    return asi_compare(str1,str2);
}

static int anna_is_string(anna_entry_t e)
{
    if(anna_entry_null(e))
    {
	return 0;
    }
    if(!anna_is_obj(e))
    {
	return 0;
    }
    anna_object_t *obj = anna_as_obj_fast(e);
    return !!obj->type->mid_identifier[ANNA_MID_STRING_PAYLOAD];
}


ANNA_VM_NATIVE(anna_string_cmp_i, 2)
{
    anna_entry_t res = null_entry;
    if(likely(anna_is_string(param[1])))
    {
	anna_object_t *this = anna_as_obj(param[0]);
	anna_object_t *that = anna_as_obj(param[1]);
	res = anna_from_int(anna_string_cmp(this, that));	
    }
    return res;
}

int anna_string_hash(anna_object_t *this)
{
    anna_string_t *s = as_unwrap(this);
    size_t l = asi_get_count(s);
    size_t i;
    unsigned hash = 5381 ^ anna_string_seed;
    
    for(i=0; i<l; i++){
	wchar_t ch = asi_get_char(s, i);
	hash = ((hash << 5) + hash) ^ ch; 
    }
//    anna_message(L"%ls => %d\n", asi_cstring(s), hash);  
    return hash;
}

ANNA_VM_NATIVE(anna_string_hash_i, 1)
{
    return anna_from_int(anna_string_hash(anna_as_obj_fast(param[0])));
}

static void anna_string_iterator_update(anna_object_t *iter, int off)
{
    anna_entry_set(iter, ANNA_MID_KEY, anna_from_int(off));
}

ANNA_VM_NATIVE(anna_string_get_iterator, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *string = anna_as_obj(param[0]);
    anna_object_t *iter = anna_object_create(
	anna_type_unwrap(anna_entry_get_static_obj(string->type, ANNA_MID_ITERATOR_TYPE)));
    anna_entry_set(iter, ANNA_MID_COLLECTION, param[0]);
    anna_string_iterator_update(iter, 0);
    return anna_from_obj(iter);
}

ANNA_VM_NATIVE(anna_string_iterator_next, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *iter = anna_as_obj(param[0]);
    anna_string_iterator_update(iter, anna_as_int(anna_entry_get(iter, ANNA_MID_KEY))+1);
    return param[0];
}

ANNA_VM_NATIVE(anna_string_iterator_valid, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *iter = anna_as_obj(param[0]);
    anna_object_t *string = anna_as_obj(anna_entry_get(iter, ANNA_MID_COLLECTION));
    int offset = anna_as_int(anna_entry_get(iter, ANNA_MID_KEY));
    return (offset >= 0  && offset < anna_string_get_count(string)) ? anna_from_int(1) : null_entry;
}


ANNA_VM_NATIVE(anna_string_iterator_get_value, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *iter = anna_as_obj(param[0]);
    anna_object_t *string = anna_as_obj(anna_entry_get(iter, ANNA_MID_COLLECTION));
    int off = anna_as_int(anna_entry_get(iter, ANNA_MID_KEY));

    if((off >= 0) && (off < anna_string_get_count(string)))
    {
	return anna_from_char(asi_get_char(as_unwrap(string), off));
    }
    else
    {
	return null_entry;
    }
}

ANNA_VM_NATIVE(anna_string_iterator_set_value, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    ANNA_ENTRY_NULL_CHECK(param[1]);

    anna_object_t *iter = anna_as_obj(param[0]);
    anna_object_t *string = anna_as_obj(anna_entry_get(iter, ANNA_MID_COLLECTION));
    int off = anna_as_int(anna_entry_get(iter, ANNA_MID_KEY));

    asi_set_char(as_unwrap(string), off, anna_as_char(param[1]));
    return param[1];
}

static anna_type_t *anna_string_iterator_create(
    anna_type_t *type,
    int mutable)
{
    anna_type_t *iter = anna_type_create(L"Iterator", 0);
    anna_member_create(
	iter, ANNA_MID_COLLECTION, ANNA_MEMBER_IMUTABLE, type);
    anna_member_create(
	iter, ANNA_MID_KEY, ANNA_MEMBER_IMUTABLE, int_type);
    anna_type_copy_object(iter);
    
    anna_member_create_native_property(
	iter, ANNA_MID_VALID, any_type,
	&anna_string_iterator_valid,
	0, 0);
    
    anna_member_create_native_property(
	iter, ANNA_MID_VALUE, char_type,
	&anna_string_iterator_get_value,
	mutable ? &anna_string_iterator_set_value : 0,
	0);
    
    anna_type_t *iter_argv[] = 
	{
	    iter
	}
    ;
    
    wchar_t *iter_argn[]=
	{
	    L"this"
	}
    ;

    anna_member_create_native_method(
	iter,
	ANNA_MID_NEXT_ASSIGN, 0,
	&anna_string_iterator_next, iter, 1,
	iter_argv, iter_argn, 0, 0);

    anna_util_iterator_iterator(iter);
        
    anna_type_close(iter);
    
    return iter;
}


static void anna_string_type_create_internal(anna_type_t *type, int mutable)
{
    mid_t mmid;
    anna_function_t *fun;
    
    anna_member_create_blob(
	type, ANNA_MID_STRING_PAYLOAD, 0,
	sizeof(anna_string_t));
    
    anna_member_create(
	type,
	ANNA_MID_ITERATOR_TYPE,
	ANNA_MEMBER_STATIC,
	type_type);
    anna_type_t *iter = anna_string_iterator_create(type, mutable);
    anna_entry_set_static(
	type, ANNA_MID_ITERATOR_TYPE, 
	anna_from_obj(anna_type_wrap(iter)));
    anna_member_create_native_property(
	type, ANNA_MID_ITERATOR, iter,
	&anna_string_get_iterator, 0,
	L"Returns an Iterator for this collection.");

    anna_type_t *i_argv[] = 
	{
	    type,
	    int_type,
	    char_type
	}
    ;
    
    wchar_t *i_argn[] =
	{
	    L"this", L"index", L"value"
	}
    ;
    
    anna_type_t *c_argv[] = 
	{
	    type,
	    string_type
	}
    ;
    
    wchar_t *c_argn[] =
	{
	    L"this", L"other"
	}
    ;
    
    anna_type_t *o_argv[] = 
	{
	    type,
	    any_type
	}
    ;
    
    wchar_t *o_argn[] =
	{
	    L"this", L"value"
	}
    ;
    
    anna_member_create_native_method(
	type, ANNA_MID_INIT,
	0, &anna_string_init, type, 1,
	o_argv, o_argn, 0, 0);
    
    anna_type_finalizer_add(
	type, anna_string_del);
    
    anna_member_create_native_method(
	type, ANNA_MID_CMP,
	0, &anna_string_cmp_i, int_type, 2,
	c_argv, c_argn, 0, 0);
    
    mmid = anna_member_create_native_method(
	type,
	ANNA_MID_GET, 0,
	&anna_string_i_get_int,
	char_type,
	2, i_argv, i_argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(type, mmid)));
    anna_member_alias(type, mmid, L"__get__");
    
    wchar_t *join_argn[] =
	{
	    L"this", L"other"
	}
    ;
    
    anna_type_t *join_argv[] = 
	{
	    type,
	    any_type
	}
    ;

    mmid = anna_member_create_native_method(
	type,
	anna_mid_get(L"__join__"), 0,
	&anna_string_i_join, type,
	2, join_argv, join_argn, 0, L"The join operator is used to create a new string that is the concatenation of the first string and the string representation of the other object");
    anna_member_document(
	type, mmid,
	L"The join operator can be used like this:");
    anna_member_document_example(
	type, mmid, 
	L"myString := \"Hello, \" ~ userName");

    if(!mutable)
    {
	fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(type, mmid)));
	fun->flags |= ANNA_FUNCTION_PURE;
    }
    
    wchar_t *ljoin_argn[] =
	{
	    L"this", L"list"
	}
    ;
    
    anna_type_t *ljoin_argv[] = 
	{
	    type,
	    any_list_type
	}
    ;

    mmid = anna_member_create_native_method(
	type, anna_mid_get(L"join"), 0,
	&anna_string_i_ljoin, type, 2,
	ljoin_argv, ljoin_argn, 0, 
	L"Concatenate each element of the specified list by using this string as the separator.");
    anna_member_document(
	type, mmid,
	L"For example:");
    anna_member_document_example(
	type, mmid, 
	L"\":\".join([1,2,3]) // Results in the string 1:2:3");
    
    mmid = anna_member_create_native_method(
	type,
	anna_mid_get(L"__appendAssign__"),
	0,
	mutable ? &anna_string_i_append : anna_string_i_join,
	type,
	2, join_argv, join_argn, 0, 0);

/*
    wchar_t *ac_argn[] =
	{
	    L"this", L"other"
	}
    ;
    
    anna_type_t *ac_argv[] = 
	{
	    string_type,
	    char_type
	}
    ;
*/
    anna_member_create_native_property(
	type, ANNA_MID_COUNT,
	int_type, &anna_string_i_get_count,
	mutable?&anna_string_i_set_count:0,
	L"The number of characters in this String.");

    anna_member_create_native_property(
	type, anna_mid_get(L"lower"),
	type, &anna_string_i_get_lower, 0,
	L"Returns a lower case version of this string.");

    anna_member_create_native_property(
	type, anna_mid_get(L"upper"),
	type, &anna_string_i_get_upper, 0,
	L"Returns a upper case version of this string.");

    anna_type_t *range_argv[] = 
	{
	    type,
	    range_type,
	    string_type
	}
    ;

    wchar_t *range_argn[] =
	{
	    L"this", L"range", L"value"
	}
    ;
    
    mmid = anna_member_create_native_method(
	type,
	ANNA_MID_GET_RANGE,
	0,
	&anna_string_i_get_range,
	type,
	2,
	range_argv,
	range_argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(type, mmid)));
    anna_member_alias(type, mmid, L"__get__");
    
    anna_type_t *mul_argv[] = 
	{
	    type,
	    int_type
	}
    ;

    wchar_t *mul_argn[] =
	{
	    L"this", L"times"
	}
    ;
    
    mmid = anna_member_create_native_method(
	type,
	anna_mid_get(L"__mul__"),
	0,
	&anna_string_i_mul,
	type,
	2,
	mul_argv,
	mul_argn, 0, L"Returns this string repeated the specified number of times.");
    
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(type, mmid)));
    anna_member_alias_reverse(type, mmid, L"__mul__");

    anna_member_create_native_method(
	type, ANNA_MID_TO_STRING, 0,
	&anna_string_to_string, string_type, 1,
	i_argv, i_argn, 0, L"Returns this string.");
    
    if(mutable)
    {
	mmid = anna_member_create_native_method(
	    type,
	    ANNA_MID_SET, 0,
	    &anna_string_i_set_int,
	    char_type,
	    3,
	    i_argv,
	    i_argn, 0, 0);
	fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(type, mmid)));
	anna_member_alias(type, mmid, L"__set__");
	
	mmid = anna_member_create_native_method(
	    type,
	    ANNA_MID_SET_RANGE,
	    0,
	    &anna_string_i_set_range,
	    type,
	    3,
	    range_argv,
	    range_argn, 0, 0);
	fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(type, mmid)));
	anna_member_alias(type, mmid, L"__set__");
    }
    anna_member_create_native_method(
	type,
	ANNA_MID_HASH_CODE,
	0,
	&anna_string_hash_i,
	int_type,
	1,
	i_argv,
	i_argn, 0, L"Calculate the hashcode for this string.");
    
    anna_member_create_native_property(
	type, ANNA_MID_FREEZE,
	imutable_string_type, mutable ? &anna_string_i_copy : &anna_util_noop,
	0,
    	L"An imutable copy of this String, or the String itself if it is already imutable.");
    anna_member_create_native_property(
	type, ANNA_MID_THAW,
	mutable_string_type, mutable ? &anna_util_noop : &anna_mutable_string_i_copy,
	0,
	L"A mutable copy of this String, or the String itself if it is already mutable.");
}

void anna_string_type_create()
{
    anna_string_seed = time(0);
    
    anna_string_type_create_internal(imutable_string_type, 0);
    anna_string_type_create_internal(mutable_string_type, 1);
    anna_type_intersect_into(
	string_type, imutable_string_type, mutable_string_type);

    anna_type_make_sendable(imutable_string_type);
        
    wchar_t *conv_argn[]=
	{
	    L"value"
	}
    ;
    
    anna_member_create_native_type_method(
	string_type, anna_mid_get(L"convert"),
	0, &anna_string_convert, string_type,
	1, &any_type, conv_argn, 0, L"Convert any object into a String. This is done by calling the toString method of the Object.");

    anna_type_document(
	string_type,
	L"The String type represents any character string, either a mutable or imutable one. It is the intersection of the MutableString and the ImutableString.");
    
    anna_type_document(
	string_type,
	L"The String type is an abstract type, it should never be instantiated.");
    
    anna_type_document(
	mutable_string_type,
	L"The MutableString type is type representing a mutable (changing) character string.");
    
    anna_type_document(
	mutable_string_type,
	L"In order to obtain an imutable (unchangable) version of a MutableString, use the freeze property. On ImutableString, this property returns the original string.");
    
    anna_type_document(
	imutable_string_type,
	L"The ImutableString type is type representing an imutable (unchanging) character string.");
    
    anna_type_document(
	imutable_string_type,
	L"In order to obtain a mutable (changable) version of an ImutableString, use the thaw property. On MutableString, this property returns the original string.");
    
    anna_type_document(
	imutable_string_type,
	L"String literals, such as \"test\" are all instances of ImutableString. In order to obtain a mutable (changable) version of an ImutableString, use the thaw property.");
    
}
