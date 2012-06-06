static anna_object_t *anna_range_create(
    int from,
    int to,
    int step,
    int open)
{
    anna_object_t *range = anna_object_create(range_type);

    anna_range_set_from(range, from);
    anna_range_set_to(range, to);
    anna_range_set_step(range, step);
    anna_range_set_open(range, open);
    return range;
}


ssize_t anna_range_get_from(anna_object_t *obj)
{
    return *(ssize_t *)anna_entry_get_addr(obj,ANNA_MID_RANGE_FROM);    
}

ssize_t anna_range_get_to(anna_object_t *obj)
{
    return *(ssize_t *)anna_entry_get_addr(obj,ANNA_MID_RANGE_TO);
}

ssize_t anna_range_get_step(anna_object_t *obj)
{
    return *(ssize_t *)anna_entry_get_addr(obj,ANNA_MID_RANGE_STEP);    
}

int anna_range_get_open(anna_object_t *obj)
{
    return *(int *)anna_entry_get_addr(obj,ANNA_MID_RANGE_OPEN);    
}

static int anna_range_is_valid(anna_object_t *obj)
{
    ssize_t from = anna_range_get_from(obj);
    ssize_t to = anna_range_get_to(obj);
    ssize_t step = anna_range_get_step(obj);
    return (to>from)==(step>0);
}

ssize_t anna_range_get_count(anna_object_t *obj)
{
    ssize_t from = anna_range_get_from(obj);
    ssize_t to = anna_range_get_to(obj);
    ssize_t step = anna_range_get_step(obj);
    return (1+(to-from-sign(step))/step);
}

void anna_range_set_from(anna_object_t *obj, ssize_t v)
{
    *((ssize_t *)anna_entry_get_addr(obj,ANNA_MID_RANGE_FROM)) = v;
}

void anna_range_set_to(anna_object_t *obj, ssize_t v)
{
    *((ssize_t *)anna_entry_get_addr(obj,ANNA_MID_RANGE_TO)) = v;
}

void anna_range_set_step(anna_object_t *obj, ssize_t v)
{
    if(v != 0)
	*((ssize_t *)anna_entry_get_addr(obj,ANNA_MID_RANGE_STEP)) = v;
}

void anna_range_set_open(anna_object_t *obj, int v)
{
    *((int *)anna_entry_get_addr(obj,ANNA_MID_RANGE_OPEN)) = !!v;
}

ANNA_VM_NATIVE(anna_range_get_from_i, 1)
{
    return anna_from_int(anna_range_get_from(anna_as_obj_fast(param[0])));
}

ANNA_VM_NATIVE(anna_range_get_to_i, 1)
{
    anna_object_t *r = anna_as_obj_fast(param[0]);
    return anna_range_get_open(r)?null_entry:anna_from_int(anna_range_get_to(r));
}

ANNA_VM_NATIVE(anna_range_get_step_i, 1)
{
    anna_object_t *r = anna_as_obj_fast(param[0]);
    return anna_from_int(anna_range_get_step(r));
}

ANNA_VM_NATIVE(anna_range_get_first_i, 1)
{
    anna_object_t *r = anna_as_obj_fast(param[0]);
    return
	anna_range_is_valid(r)?anna_from_int(anna_range_get_from(r)):null_entry;
}

ANNA_VM_NATIVE(anna_range_get_last_i, 1)
{
    anna_object_t *r = anna_as_obj_fast(param[0]);
    
    if(!anna_range_is_valid(r) || anna_range_get_open(r))
    {
	return null_entry;
    }
    else 
    {
	ssize_t from = anna_range_get_from(r);
	ssize_t step = anna_range_get_step(r);
	return anna_from_int(from + step*(anna_range_get_count(r)-1));
    }    
}

ANNA_VM_NATIVE(anna_range_get_open_i, 1)
{
    anna_object_t *r = anna_as_obj_fast(param[0]);
    return anna_range_get_open(r)?anna_from_int(1):null_entry;
}

ANNA_VM_NATIVE(anna_range_init, 4)
{
    anna_object_t *range = anna_as_obj_fast(param[0]);

    ssize_t from = anna_entry_null(param[1])?0:anna_as_int(param[1]);
    anna_range_set_from(range, from);
    ssize_t to = anna_entry_null(param[2])?0:anna_as_int(param[2]);
    anna_range_set_to(range, to);
    int open = anna_entry_null(param[2]);
    anna_range_set_open(range, open);

    if(anna_entry_null(param[3]))
    {
	anna_range_set_step(range, ((to>from)|| open)?1:-1);
    }
    else
    {
	ssize_t step = anna_as_int(param[3]);
	anna_range_set_step(range, step != 0 ? step:1);
    }
    return anna_from_obj(range);
}

ANNA_VM_NATIVE(anna_range_get_int, 2)
{  
    anna_object_t *range = anna_as_obj_fast(param[0]);
    ssize_t from = anna_range_get_from(range);
    ssize_t step = anna_range_get_step(range);
    ssize_t idx = anna_as_int(param[1]);
    int open = anna_range_get_open(range);
    if(open)
    {
	if(idx >= 0)
	{
	    return anna_from_int(from + step*idx);
	}
    }
    else
    {
	idx = anna_list_calc_offset(idx, anna_range_get_count(range));
	if(likely(idx >= 0)){
	    return anna_from_int(from + step*idx);
	}
    }
    return null_entry;
}

ANNA_VM_NATIVE(anna_range_get_range, 2)
{  
    anna_object_t *range = anna_as_obj_fast(param[0]);
    ssize_t from = anna_range_get_from(range);
    ssize_t to = anna_range_get_to(range);
    ssize_t step = anna_range_get_step(range);
    int open = anna_range_get_open(range);

    anna_object_t *idx = anna_as_obj(param[1]);
    
    ssize_t idx_from = anna_range_get_from(idx);
    ssize_t idx_to = anna_range_get_to(idx);
    ssize_t idx_step = anna_range_get_step(idx);
    int idx_open = anna_range_get_open(idx);

    if(open)
    {
	if(idx_from < 0 || idx_to < 0)
	{
	    return null_entry;
	}
	
	if(idx_open)
	{	    
	    if(idx_step < 0)
	    {
		return anna_from_obj(anna_range_create(
					 from + step*idx_from,
					 from + sign(step)*sign(idx_step),
					 step*idx_step,
					 0));
	    }
	    return anna_from_obj(anna_range_create(
		from + step*idx_from,
		0,
		step*idx_step,
		1));
	}
	else
	{
	    return anna_from_obj(anna_range_create(
		from + step*idx_from,
		from + step*idx_to,
		step*idx_step,
		0));
	}
    }
    else
    {
	idx_from = anna_list_calc_offset(idx_from, anna_range_get_count(range));
	if(idx_from < 0)
	{
	    return null_entry;
	}
	if(idx_from >= to)
	{
	    return null_entry;
	}
	
	if(idx_open)
	{
	    return anna_from_obj(
		anna_range_create(
		    from + step*idx_from,
		    to,
		    step*idx_step,
		    0));
	}
	else
	{
	    idx_to = anna_list_calc_offset(idx_to, anna_range_get_count(range));
	    if(idx_to < 0)
	    {
		return null_entry;
	    }
	    idx_step = (idx_to > idx_from ? 1 : -1) * abs(idx_step);	

	    if(idx_to > to)
	    {
		return null_entry;
	    }
	    
	    return anna_from_obj(
		anna_range_create(
		    from + step*idx_from,
		    from + step*idx_step*idx_to,
		    step*idx_step,
		    0));
	}

    }
    return null_entry;
}

ANNA_VM_NATIVE(anna_range_in, 2)
{
    anna_object_t *range = anna_as_obj_fast(param[0]);
    ANNA_ENTRY_NULL_CHECK(param[1]);
    ssize_t from = anna_range_get_from(range);
    ssize_t to = anna_range_get_to(range);
    ssize_t step = anna_range_get_step(range);
    int open = anna_range_get_open(range);
    int val = anna_as_int(param[1]);
    if(step > 0)
    {
	if(val < from)
	{
	    return null_entry;
	}
	if((val >= to) && !open)
	{
	    return null_entry;
	}
	int rem = (val-from)%step;
	int res = (val-from)/step;
	return (rem == 0)?anna_from_int(res):null_entry;
    }
    else
    {
	if(val > from)
	{
	    return null_entry;
	}
	if((val <= to) && !open)
	{
	    return null_entry;
	}
	int rem = (val-from)%step;
	int res = (val-from)/step;
	return (rem == 0)?anna_from_int(res):null_entry;
    }
}

ANNA_VM_NATIVE(anna_range_get_count_i, 1)
{
    anna_object_t *range = anna_as_obj_fast(param[0]);
    return anna_range_get_open(range)? null_entry:(anna_range_is_valid(range)?anna_from_int(anna_range_get_count(range)):null_entry);
}

/**
   This is the bulk of the each method
 */
static void anna_range_each_callback_closed(anna_context_t *context)
{
    // Discard the output of the previous method call
    anna_context_pop_entry(context);
    // Set up the param list. These are the values that aren't reallocated each lap
    anna_entry_t **param = context->top - 4;
    // Unwrap and name the params to make things more explicit
    anna_object_t *range = anna_as_obj_fast(param[0]);
    anna_object_t *body =  anna_as_obj_fast(param[1]);
    int idx = anna_as_int(param[2]);
    int count = anna_as_int(param[3]);
    
    ssize_t from = anna_range_get_from(range);
    ssize_t step = anna_range_get_step(range);

    //anna_message(L"BBB %d %d\n", context->frame, context->frame->flags);
    // Are we done or do we need another lap?
    if(context->frame->flags & ANNA_ACTIVATION_FRAME_BREAK)
    {
	anna_context_drop(context, 4);
	anna_context_push_object(context, range);	
    }
    else if(idx < count)
    {
	// Set up params for the next lap of the each body function
	anna_entry_t *o_param[] =
	    {
		param[2],
		anna_from_int(from + step*idx)
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
	anna_context_push_object(context, range);
    }
}

static void anna_range_each_callback_open(anna_context_t *context)
{
    // Discard the output of the previous method call
    anna_context_pop_entry(context);
    // Set up the param list. These are the values that aren't reallocated each lap
    anna_entry_t **param = context->top - 4;
    // Unwrap and name the params to make things more explicit
    anna_object_t *range = anna_as_obj_fast(param[0]);
    anna_object_t *body =  anna_as_obj_fast(param[1]);
    int idx = anna_as_int(param[2]);
    
    ssize_t from = anna_range_get_from(range);
    ssize_t step = anna_range_get_step(range);

    if(context->frame->flags & ANNA_ACTIVATION_FRAME_BREAK)
    {
	anna_context_drop(context, 4);
	anna_context_push_object(context, range);	
    }
    else
    {
	
	// Set up params for the next lap of the each body function
	anna_entry_t *o_param[] =
	    {
		param[2],
	    anna_from_int(from + step*idx)
	    }
	;
	// Then update our internal lap counter
	param[2] = anna_from_int(idx+1);
	
	// Finally, roll the code point back a bit and push new arguments
	anna_vm_callback_reset(context, body, 2, o_param);
    }
}

static void anna_range_each(anna_context_t *context)
{
    anna_object_t *body = anna_context_pop_object(context);
    anna_object_t *range = anna_context_pop_object(context);
    anna_context_pop_entry(context);
    
    ssize_t from = anna_range_get_from(range);
    ssize_t to = anna_range_get_to(range);
    ssize_t step = anna_range_get_step(range);
    int open = anna_range_get_open(range);
    ssize_t count = 1+(to-from-sign(step))/step;
    
    if((count<=0) && !open)
    {
	anna_context_push_object(context, range);
    }
    else
    {
	anna_entry_t *callback_param[] = 
	    {
		anna_from_obj(range),
		anna_from_obj(body),
		anna_from_int(1),
		anna_from_int(count)
	    }
	;
	
	anna_entry_t *o_param[] =
	    {
		anna_from_int(0),
		anna_from_int(from)
	    }
	;
	anna_native_t callback = open ? anna_range_each_callback_open : 
	    anna_range_each_callback_closed;
	
	anna_vm_callback_native(
	    context,
	    callback, 4, callback_param,
	    body, 2, o_param
	    );
    }
}

static inline anna_entry_t *anna_range_get(anna_object_t *this, ssize_t idx)
{
    return anna_from_int(anna_range_get_from(this) + idx * anna_range_get_step(this));
}

static void anna_range_filter_callback(anna_context_t *context)
{
    anna_entry_t *value = anna_context_pop_entry(context);

    anna_entry_t **param = context->top - 4;
    anna_object_t *range = anna_as_obj_fast(param[0]);
    anna_object_t *body =  anna_as_obj_fast(param[1]);
    int idx = anna_as_int(param[2]);
    anna_object_t *res = anna_as_obj_fast(param[3]);
    size_t sz = anna_range_get_count(range);
    int open = anna_range_get_open(range);
    
    if(!anna_entry_null(value))
    {
	anna_list_add(res, anna_range_get(range, idx-1));
    }
    
    if( ((sz > idx) || open) && (!(context->frame->flags & ANNA_ACTIVATION_FRAME_BREAK)))
    {
	anna_entry_t *o_param[] =
	    {
		param[2],
		anna_range_get(range, idx)
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

static void anna_range_filter(anna_context_t *context)
{
    anna_object_t *res = anna_list_create_mutable(int_type);
    anna_object_t *body = anna_context_pop_object(context);
    anna_object_t *range = anna_context_pop_object(context);
    anna_context_pop_entry(context);
    
    size_t sz = anna_range_get_count(range);
    int open = anna_range_get_open(range);
    
    if(sz > 0 || open)
    {
	anna_entry_t *callback_param[] = 
	    {
		anna_from_obj(range),
		anna_from_obj(body),
		anna_from_int(1),
		anna_from_obj(res)
	    }
	;
	
	anna_entry_t *o_param[] =
	    {
		anna_from_int(0),
		anna_from_int(anna_range_get_from(range))
	    }
	;
	
	anna_vm_callback_native(
	    context,
	    anna_range_filter_callback, 4, callback_param,
	    body, 2, o_param
	    );
    }
    else
    {
	anna_context_push_object(context, res);
    }
}

static void anna_range_find_callback(anna_context_t *context)
{
    anna_entry_t *value = anna_context_pop_entry(context);
    
    anna_entry_t **param = context->top - 3;
    anna_object_t *range = anna_as_obj_fast(param[0]);
    anna_object_t *body = anna_as_obj_fast(param[1]);
    int idx = anna_as_int(param[2]);
    size_t sz = anna_range_get_count(range);
    int open = anna_range_get_open(range);
    
    if(!anna_entry_null(value))
    {
	anna_context_drop(context, 4);
	anna_context_push_entry(context, anna_range_get(range, idx-1));	
    }
    else if((sz > idx || open) && !(context->frame->flags & ANNA_ACTIVATION_FRAME_BREAK))
    {
	anna_entry_t *o_param[] =
	    {
		param[2],
		anna_range_get(range, idx)
	    }
	;
	
	param[2] = anna_from_int(idx+1);
	anna_vm_callback_reset(context, body, 2, o_param);
    }
    else
    {
	anna_context_drop(context, 4);
	anna_context_push_object(context, null_object);
    }    
}

static void anna_range_find(anna_context_t *context)
{
    anna_object_t *body = anna_context_pop_object(context);
    anna_object_t *range = anna_context_pop_object(context);
    anna_context_pop_entry(context);
    
    size_t sz = anna_range_get_count(range);
    int open = anna_range_get_open(range);
    
    if(sz > 0 || open)
    {
	anna_entry_t *callback_param[] = 
	    {
		anna_from_obj(range),
		anna_from_obj(body),
		anna_from_int(1),
	    }
	;
	
	anna_entry_t *o_param[] =
	    {
		anna_from_int(0),
		anna_from_int(anna_range_get_from(range))
	    }
	;
	
	anna_vm_callback_native(
	    context,
	    anna_range_find_callback, 3, callback_param,
	    body, 2, o_param
	    );
    }
    else
    {
	anna_context_push_object(context, null_object);
    }
}

static void anna_range_map_callback(anna_context_t *context)
{
    anna_entry_t *value = anna_context_pop_entry(context);

    anna_entry_t **param = context->top - 4;
    anna_object_t *range = anna_as_obj_fast(param[0]);
    anna_object_t *body = anna_as_obj_fast(param[1]);
    int idx = anna_as_int(param[2]);
    anna_object_t *res = anna_as_obj_fast(param[3]);
    size_t sz = anna_range_get_count(range);
    int open = anna_range_get_open(range);
    
    anna_list_set(res, idx-1, value);
    
    if((sz > idx || open) && !(context->frame->flags & ANNA_ACTIVATION_FRAME_BREAK))
    {
	anna_entry_t *o_param[] =
	    {
		param[2],
		anna_range_get(range, idx)
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

static void anna_range_map(anna_context_t *context)
{
    anna_object_t *body = anna_context_pop_object(context);
    anna_object_t *range = anna_context_pop_object(context);
    anna_context_pop_entry(context);
    if(body == null_object)
    {
	anna_context_push_object(context, null_object);
    }
    else
    {
	anna_function_t *fun = anna_function_unwrap(body);
	anna_object_t *res = anna_list_create_mutable(fun->return_type);
	
	size_t sz = anna_range_get_count(range);
	int open = anna_range_get_open(range);

	if(sz > 0 || open)
	{
	    anna_entry_t *callback_param[] = 
		{
		    anna_from_obj(range),
		    anna_from_obj(body),
		    anna_from_int(1),
		    anna_from_obj(res)
		}
	    ;
	    
	    anna_entry_t *o_param[] =
		{
		    anna_from_int(0),
		    anna_range_get(range, 0)
		}
	    ;
	    
	    anna_vm_callback_native(
		context,
		anna_range_map_callback, 4, callback_param,
		body, 2, o_param
		);
	}
	else
	{
	    anna_context_push_object(context, res);
	}
    }
}

void anna_range_type_create()
{
    mid_t mmid;

    anna_type_make_sendable(range_type);

    anna_member_create(range_type, ANNA_MID_RANGE_FROM, 0, null_type);
    anna_member_create(
	range_type,
	ANNA_MID_RANGE_TO,
	0,
	null_type);
    anna_member_create(
	range_type,
	ANNA_MID_RANGE_STEP,
	0,
	null_type);
    anna_member_create(
	range_type,
	ANNA_MID_RANGE_OPEN,
	0,
	null_type);    
    
    anna_type_t *c_argv[] = 
	{
	    range_type,
	    int_type,
	    int_type,
	    int_type,
	    object_type
	}
    ;
    
    wchar_t *c_argn[]=
	{
	    L"this", L"from", L"to", L"step", L"open?"
	}
    ;

    anna_member_create_native_method(
	range_type, anna_mid_get(L"__init__"),
	0, &anna_range_init, range_type, 4,
	c_argv, c_argn, 0, L"Constructs a Range object with the specified parameters,");    

    anna_type_t *i_argv[] = 
	{
	    range_type,
	    int_type
	}
    ;

    wchar_t *i_argn[]=
	{
	    L"this", L"index"
	}
    ;

    mmid = anna_member_create_native_method(
	range_type,
	anna_mid_get(L"get"), 0,
	&anna_range_get_int, int_type, 2,
	i_argv, i_argn, 0, 0);
    anna_member_alias(range_type, mmid, L"__get__");
    
    anna_type_t *range_argv[] = 
	{
	    range_type,
	    range_type
	}
    ;

    wchar_t *range_argn[]=
	{
	    L"this", L"index"
	}
    ;

    mmid = anna_member_create_native_method(
	range_type,
	anna_mid_get(L"getRange"), 0,
	&anna_range_get_range, range_type, 2,
	range_argv, range_argn, 0, 0);
    anna_member_alias(range_type, mmid, L"__get__");

    anna_member_create_native_property(
	range_type, anna_mid_get(L"count"), int_type,
	&anna_range_get_count_i, 0, L"The number of elements in this Range.");
    anna_member_create_native_property(
	range_type,
	anna_mid_get(L"from"),
	int_type,
	&anna_range_get_from_i,
	0, L"The first element in this Range.");
    anna_member_create_native_property(
	range_type,
	anna_mid_get(L"to"),
	int_type,
	&anna_range_get_to_i,
	0, L"The last element in this Range.");
    anna_member_create_native_property(
	range_type,
	anna_mid_get(L"step"),
	int_type,
	&anna_range_get_step_i,
	0, L"The distance between elements in this Range.");
    anna_member_create_native_property(
	range_type,
	anna_mid_get(L"open?"),
	int_type,
	&anna_range_get_open_i,
	0,
	L"Is this range semi-infinite?");
    anna_member_create_native_property(
	range_type,
	anna_mid_get(L"first"),
	int_type,
	&anna_range_get_first_i,
	0, L"The first element of this Range.");
    anna_member_create_native_property(
	range_type,
	anna_mid_get(L"last"),
	int_type,
	&anna_range_get_last_i,
	0, L"The last element of this Range.");
    
    anna_type_t *fun_type = anna_type_get_iterator(
	L"!RangeIterFunction", int_type, int_type);

    anna_type_t *e_argv[] = 
	{
	    range_type,
	    fun_type
	}
    ;
    
    wchar_t *e_argn[]=
	{
	    L"this", L"block"
	}
    ;    
    
    anna_type_t *a_argv[] = 
	{
	    range_type,
	    int_type
	}
    ;
    
    wchar_t *a_argn[]=
	{
	    L"this", L"value"
	}
    ;

    anna_member_create_native_method(
	range_type, anna_mid_get(L"each"),
	0, &anna_range_each, range_type, 2,
	e_argv, e_argn, 0, 0);
    anna_member_create_native_method(
	range_type,
	anna_mid_get(L"filter"),
	0,
	&anna_range_filter,
	anna_list_type_get_mutable(int_type),
	2,
	e_argv,
	e_argn, 0, 0);
    anna_member_create_native_method(
	range_type,
	anna_mid_get(L"find"),
	0,
	&anna_range_find,
	int_type,
	2,
	e_argv,
	e_argn, 0, 0);
    anna_member_create_native_method(
	range_type,
	anna_mid_get(L"__in__"),
	0,
	&anna_range_in,
	int_type,
	2,
	a_argv,
	a_argn, 0, L"Checks whether the specified number is contained in this range.");
    anna_member_create_native_method(
	range_type,
	anna_mid_get(L"map"),
	0,
	&anna_range_map,
	mutable_list_type,
	2,
	e_argv,
	e_argn, 0, 0);
    
    anna_type_document(
	range_type,
	L"A Range represents a sequence of equally spaced integers.");

    anna_type_document(
	range_type,
	L"Ranges are usually created using the .. and ... operators, such as <code>0..5</code> or <code>14...</code>.");



}
