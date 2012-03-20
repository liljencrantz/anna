#include "src/lib/lang/string_internal.c"

static inline anna_string_t *as_unwrap(anna_object_t *obj)
{
    return (anna_string_t *)anna_entry_get_addr(obj,ANNA_MID_STRING_PAYLOAD);
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

void anna_string_append_cstring(anna_object_t *this, size_t len, wchar_t *str)
{
    asi_append_cstring(as_unwrap(this), str, len);
}


anna_object_t *anna_string_create(size_t sz, wchar_t *data)
{
    anna_object_t *obj= anna_object_create(imutable_string_type);
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
    
    anna_object_t *obj= anna_object_create(imutable_string_type);
    // anna_message(L"Create new string \"%.*ls\" at %d\n", sz, data, obj);
    
    asi_init_from_ptr(as_unwrap(obj),  wdata, wsz);
    return obj;
}

anna_object_t *anna_string_copy(anna_object_t *orig)
{
    anna_object_t *obj= anna_object_create(imutable_string_type);
    //  anna_message(L"Create new string \"%.*ls\" at %d\n", sz, data, obj);
    
    asi_init(as_unwrap(obj));
    anna_string_t *o = as_unwrap(orig);
    asi_append(as_unwrap(obj), o, 0, asi_get_count(o));
    return obj;
}

static anna_object_t *anna_mutable_string_copy(anna_object_t *orig)
{
    anna_object_t *obj= anna_object_create(mutable_string_type);
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
    if(likely(!anna_entry_null(param[1]) && !anna_entry_null(param[2])))
    {
	anna_object_t *this = anna_as_obj(param[0]);
	anna_object_t *range = anna_as_obj(param[1]);
	anna_object_t *val = anna_as_obj(param[2]);
	
	ssize_t from = anna_string_idx_wrap(this, anna_range_get_from(range));
	ssize_t to = anna_string_idx_wrap(this, anna_range_get_to(range));
	ssize_t step = anna_range_get_step(range);
	if(anna_range_get_open(range))
	{
	    to = step>0?anna_string_get_count(this):-1;
	}
	
	ssize_t count = (1+(to-from-sign(step))/step);
	
	anna_string_t *str1 = as_unwrap(this);
	anna_string_t *str2 = as_unwrap(val);
	ssize_t i;
	
	if(step==1)
	{
	    asi_replace(
		str1,
		str2,
		from,
		to-from,
		0,
		asi_get_count(as_unwrap(val)));
	}
	else
	{
	    if(count == asi_get_count(str2))
	    {
		for(i=0; i<count;i++)
		{
		    asi_set_char(
			str1,
			from + step*i,
			asi_get_char(
			    str2,
			    i));
		}   
	    }
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
    anna_entry_t *e = anna_context_pop_entry(context);
    anna_object_t *this = anna_context_pop_object(context);
    anna_context_pop_entry(context);
    
    if(anna_entry_null(e))
    {
	anna_context_push_object(context, null_object);
    }
    else if(anna_is_int_small(e))
    {	
	anna_object_t *res = anna_object_create(this->type);
	wchar_t is[32];
	swprintf(is, 32, L"%d", anna_as_int(e));
	asi_init(as_unwrap(res));
	asi_append(as_unwrap(res), as_unwrap(this), 0, asi_get_count(as_unwrap(this)));
	asi_append_cstring(as_unwrap(res), is, wcslen(is));
	
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
	    anna_entry_t *callback_param[] = 
		{
		    anna_from_obj(this),
		}
	    ;
	    
	    anna_entry_t *o_param[] =
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
	anna_string_t *str = as_unwrap(str_obj);
	res = anna_object_create(imutable_string_type);

	asi_init(as_unwrap(res));
	asi_append(as_unwrap(res), str, 0, asi_get_count(str));
    }
    anna_context_pop_entry(context);
    anna_context_push_object(context, res);
}

static void anna_string_convert(anna_context_t *context)
{
    anna_entry_t *e = anna_context_pop_entry(context);
    anna_context_pop_entry(context);
    
    if(anna_entry_null(e))
    {
	anna_context_push_entry(context, e);
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
	if((o->type == mutable_string_type) ||
	   (o->type == imutable_string_type))
	{
	    anna_context_push_object(context, o);
	}
	else
	{
	    anna_object_t *fun_object = anna_as_obj_fast(anna_entry_get_static(o->type, ANNA_MID_TO_STRING));
	    anna_entry_t *o_param[] =
		{
		    anna_from_obj(o)
		}
	    ;
	    
	    anna_vm_callback_native(
		context,
		anna_string_convert_callback, 0, 0,
		fun_object, 1, o_param
		);
	}
    }
}

static void anna_string_ljoin_callback(anna_context_t *context)
{    
    anna_object_t *value = anna_context_pop_object(context);
    anna_entry_t **param = context->top - 4;
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
	anna_vm_callback_reset(context, meth, 1, (anna_entry_t **)&o);
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
	    anna_entry_t *callback_param[] = 
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
		meth, 1, (anna_entry_t **)&o
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
    anna_entry_t **param = context->top - 1;
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
	    anna_entry_t *callback_param[] = 
		{
		    anna_from_obj(this)
		}
	    ;
	    
	    anna_member_t *tos_mem = anna_member_get(obj->type, ANNA_MID_TO_STRING);
	    anna_object_t *meth = anna_as_obj_fast(obj->type->static_member[tos_mem->offset]);
	    anna_vm_callback_native(
		context,
		anna_string_append_callback, 1, callback_param,
		meth, 1, (anna_entry_t **)&obj
		);
	}
    }
    else{
	anna_context_push_object(context, this);
    }
}

/**
   This is the bulk of the each method
 */
static void anna_string_each_callback(anna_context_t *context)
{
    // Discard the output of the previous method call
    anna_context_pop_entry(context);
    // Set up the param list. These are the values that aren't reallocated each lap
    anna_entry_t **param = context->top - 3;
    // Unwrap and name the params to make things more explicit
    anna_object_t *str_obj = anna_as_obj_fast(param[0]);
    anna_object_t *body = anna_as_obj_fast(param[1]);
    anna_string_t *str = as_unwrap(str_obj);
    int idx = anna_as_int(param[2]);
    size_t sz = asi_get_count(str);
    
    // Are we done or do we need another lap?
    if(idx < sz)
    {
	// Set up params for the next lap of the each body function
	anna_entry_t *o_param[] =
	    {
		param[2],
		anna_from_char(asi_get_char(str, idx))
	    }
	;
	// Then update our internal lap counter
	param[2] = anna_from_int(idx+1);
	
	// Finally, roll the code point back a bit and push new arguments
	anna_vm_callback_reset(context, body, 2, o_param);
    }
    else
    {
	// Oops, we're done. Drop our internal param list and push the correct output
	anna_context_drop(context, 4);
	anna_context_push_object(context, str_obj);
    }
}

static void anna_string_i_each(anna_context_t *context)
{
    anna_object_t *body = anna_context_pop_object(context);
    anna_object_t *str_obj = anna_context_pop_object(context);
    anna_context_pop_entry(context);
    anna_string_t *str = as_unwrap(str_obj);    
    size_t sz = asi_get_count(str);

    if(sz > 0)
    {
	anna_entry_t *callback_param[] = 
	    {
		anna_from_obj(str_obj),
		anna_from_obj(body),
		anna_from_int(1)
	    }
	;
	
	anna_entry_t *o_param[] =
	    {
		anna_from_int(0),
		anna_from_char(asi_get_char(str, 0))
	    }
	;
	
	anna_vm_callback_native(
	    context,
	    anna_string_each_callback, 3, callback_param,
	    body, 2, o_param
	    );
    }
    else
    {
	anna_context_push_object(context, str_obj);
    }
}




static void anna_string_i_map_callback(anna_context_t *context)
{
    anna_entry_t *value = anna_context_pop_entry(context);
    
    anna_entry_t **param = context->top - 4;
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_object_t *body = anna_as_obj_fast(param[1]);
    
    int idx = anna_as_int(param[2]);
    anna_object_t *res = anna_as_obj_fast(param[3]);
    size_t sz = anna_string_get_count(this);
    
    anna_list_set(res, idx-1, value);
    
    if( (sz > idx) && !(context->frame->flags & ANNA_ACTIVATION_FRAME_BREAK))
    {
	anna_entry_t *o_param[] =
	    {
		param[2],
		anna_from_char(asi_get_char(as_unwrap(this), idx))
	    }
	;
	
	param[2] = anna_from_int(idx+1);
	anna_vm_callback_reset(context, body, 2, o_param);
    }
    else
    {
	anna_context_drop(context, 5);
	anna_context_push_object(context, res);
    }    
}

static void anna_string_i_map(anna_context_t *context)
{
    anna_object_t *body = anna_context_pop_object(context);
    anna_object_t *this = anna_context_pop_object(context);
    anna_context_pop_entry(context);
    if(body == null_object)
    {
	anna_context_push_object(context, null_object);
    }
    else
    {
	anna_function_t *fun = anna_function_unwrap(body);
		
	anna_object_t *res = this->type == mutable_string_type?
	    anna_list_create_mutable(fun->return_type):
	    anna_list_create_imutable(fun->return_type);
	
	size_t sz = anna_string_get_count(this);
	
	if(sz > 0)
	{
	    anna_entry_t *callback_param[] = 
		{
		    anna_from_obj(this),
		    anna_from_obj(body),
		    anna_from_int(1),
		    anna_from_obj(res)
		}
	    ;
	    
	    anna_entry_t *o_param[] =
		{
		    anna_from_int(0),
		    anna_from_char(asi_get_char(as_unwrap(this), 0))
		}
	    ;
	    
	    anna_vm_callback_native(
		context,
		anna_string_i_map_callback, 4, callback_param,
		body, 2, o_param
		);
	}
	else
	{
	    anna_context_push_object(context, res);
	}
    }
}







static void anna_string_del(anna_object_t *victim)
{
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

static int anna_is_string(anna_entry_t *e)
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
    anna_entry_t *res = null_entry;
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
    unsigned hash = 5381;
    
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

static void anna_string_type_create_internal(anna_type_t *type, int mutable)
{
    mid_t mmid;
    anna_function_t *fun;

    anna_member_create_blob(
	type, ANNA_MID_STRING_PAYLOAD, 0,
	sizeof(anna_string_t));
    
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
	    object_type
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
	    object_type
	}
    ;
    
    wchar_t *o_argn[] =
	{
	    L"this", L"value"
	}
    ;
    
    anna_member_create_native_method(
	type, anna_mid_get(L"__init__"),
	0, &anna_util_noop, type, 1,
	o_argv, o_argn, 0, 0);
    
    anna_type_finalizer_add(
	type, anna_string_del);
    
    anna_member_create_native_method(
	type, anna_mid_get(L"__cmp__"),
	0, &anna_string_cmp_i, int_type, 2,
	c_argv, c_argn, 0, 0);
    
    mmid = anna_member_create_native_method(
	type,
	anna_mid_get(L"__get__Int__"), 0,
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
	    object_type
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
	L"myString := \"Hello, \" ~ userName;");

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

    anna_member_create_native_method(
	type, anna_mid_get(L"join"), 0,
	&anna_string_i_ljoin, type, 2,
	ljoin_argv, ljoin_argn, 0, 0);
    
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
	type, anna_mid_get(L"count"),
	int_type, &anna_string_i_get_count,
	mutable?&anna_string_i_set_count:0,
	L"The number of characters in this String.");

    anna_type_t *fun_type = anna_type_get_iterator(
	L"!StringIterFunction", int_type, char_type);

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
	type, anna_mid_get(L"__each__"),
	0, &anna_string_i_each, type, 2,
	e_argv, e_argn, 0, 0);

    anna_member_create_native_method(
	type,
	anna_mid_get(L"__map__"),
	0,
	&anna_string_i_map,
	mutable?mutable_list_type:imutable_list_type,
	2,
	e_argv,
	e_argn, 0, 0);
    
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
	anna_mid_get(L"__get__Range__"),
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
	i_argv, i_argn, 0, 0);
    
    if(mutable)
    {
	mmid = anna_member_create_native_method(
	    type,
	    anna_mid_get(L"__set__Int__"), 0,
	    &anna_string_i_set_int,
	    char_type,
	    3,
	    i_argv,
	    i_argn, 0, 0);
	fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(type, mmid)));
	anna_member_alias(type, mmid, L"__set__");
	
	mmid = anna_member_create_native_method(
	    type,
	    anna_mid_get(L"__set__Range__"),
	    0,
	    &anna_string_i_set_range,
	    type,
	    3,
	    range_argv,
	    range_argn, 0, 0);
	fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(type, mmid)));
	anna_member_alias(type, mmid, L"__set__");
    }
    else
    {
	anna_member_create_native_method(
	    type,
	    ANNA_MID_HASH_CODE,
	    0,
	    &anna_string_hash_i,
	    int_type,
	    1,
	    i_argv,
	    i_argn, 0, 0);
    }
    
    anna_member_create_native_property(
	type, anna_mid_get(L"freeze"),
	string_type, mutable ? &anna_string_i_copy : &anna_util_noop,
	0,
    	L"An imutable copy of this String, or the String itself if it is already imutable.");
    anna_member_create_native_property(
	type, anna_mid_get(L"thaw"),
	mutable_string_type, mutable ? &anna_util_noop : &anna_mutable_string_i_copy,
	0,
	L"A mutable copy of this String, or the String itself if it is already mutable.");
}

void anna_string_type_create()
{
    anna_string_type_create_internal(imutable_string_type, 0);
    anna_string_type_create_internal(mutable_string_type, 1);
    anna_type_intersect_into(
	string_type, imutable_string_type, mutable_string_type);
    
    wchar_t *conv_argn[]=
	{
	    L"value"
	}
    ;
    
    anna_member_create_native_type_method(
	string_type, anna_mid_get(L"convert"),
	0, &anna_string_convert, string_type,
	1, &object_type, conv_argn, 0, L"Convert any object into a String. This is done by calling the toString method of the Object.");

    anna_type_document(
	string_type,
	L"The String type represents any character string, either a mutable or imutable one. It is the intersection of the MutableString and the ImutableString.");
    
    anna_type_document(
	mutable_string_type,
	L"The MutableString type is type representing a mutable (changing) character string.");
    
    anna_type_document(
	mutable_string_type,
	L"In order to obtain an imutable (unchangable) version of a MutableString, use the freeze property. On ImutableString, this property is a returns the original string.");
    
    anna_type_document(
	imutable_string_type,
	L"The ImutableString type is type representing an imutable (unchanging) character string.");
    
    anna_type_document(
	imutable_string_type,
	L"In order to obtain a mutable (changable) version of an ImutableString, use the thaw property. On MutableString, this property is a returns the original string.");
    
    anna_type_document(
	imutable_string_type,
	L"String literals, such as \"test\" are all instances of ImutableString. In order to obtain a mutable (changable) version of an ImutableString, use the thaw property.");
    
}
