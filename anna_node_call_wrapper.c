

static inline anna_object_t *anna_node_call_wrapper_i_get_count_i(anna_object_t **param)
{
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(param[0]);
    return anna_int_create(node->child_count);
}
ANNA_VM_NATIVE(anna_node_call_wrapper_i_get_count, 1)

static inline anna_object_t *anna_node_call_wrapper_i_get_int_i(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(param[0]);
    int idx = anna_int_get(param[1]);
    if(idx < 0 || idx >= node->child_count)
	return null_object;
    return anna_node_wrap(node->child[idx]);
}
ANNA_VM_NATIVE(anna_node_call_wrapper_i_get_int, 2)

static inline anna_object_t *anna_node_call_wrapper_i_set_int_i(anna_object_t **param)
{
    
    if(param[1]==null_object)
	return null_object;
    
    if(param[2]==null_object)
	param[2] = anna_node_wrap(anna_node_create_null(0));
    
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(param[0]);
    int idx = anna_int_get(param[1]);
    if(idx < 0 || idx >= node->child_count)
	return param[1];
    
    node->child[idx] = anna_node_unwrap(param[2]);
    return param[2];
}
ANNA_VM_NATIVE(anna_node_call_wrapper_i_set_int, 3)

static inline anna_object_t *anna_node_call_wrapper_i_get_function_i(anna_object_t **param)
{
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(param[0]);
    return anna_node_wrap(node->function);
}
ANNA_VM_NATIVE(anna_node_call_wrapper_i_get_function, 1)

static inline anna_object_t *anna_node_call_wrapper_i_set_function_i(anna_object_t **param)
{
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(param[0]);
    if(param[1]==null_object)
	return null_object;
    node->function = anna_node_unwrap(param[1]);
    return param[1];    
}
ANNA_VM_NATIVE(anna_node_call_wrapper_i_set_function, 2)

static inline anna_object_t *anna_node_call_wrapper_i_join_list_i(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    size_t count = 
	anna_list_get_size(param[1]);
    
    anna_node_call_t *src = 
	(anna_node_call_t *)anna_node_unwrap(
	    param[0]);
    anna_node_call_t *dst = 
	anna_node_create_call(
	    &src->location,
	    src->function,
	    src->child_count,
	    src->child);
    int i;
    for(i=0;i<count; i++)
    {
	anna_object_t *n = 
	    anna_list_get(param[1], i);
	anna_node_call_add_child(
	    dst,
	    anna_node_unwrap(n));
    }
    return anna_node_wrap((anna_node_t *)dst);
}
ANNA_VM_NATIVE(anna_node_call_wrapper_i_join_list, 2)

static inline anna_object_t *anna_node_call_wrapper_i_init_i(anna_object_t **param)
{
    size_t sz = anna_list_get_size(param[3]);
    anna_object_t **src = anna_list_get_payload(param[3]);
    
    anna_node_t *source = anna_node_unwrap(param[1]);
    anna_node_t *function = anna_node_unwrap(param[2]);
    
    anna_node_call_t *dest = 
	anna_node_create_call(
	    &source->location,
	    function,
	    0,
	    0);
    int i;
    for(i=0; i<sz; i++)
    {
	anna_node_call_add_child(dest, anna_node_unwrap(src[i]));
    }
    dest->child_count = sz;
    
    *(anna_node_t **)anna_member_addr_get_mid(param[0],ANNA_MID_NODE_PAYLOAD)=
	(anna_node_t *)dest;
	
    return param[0];
}
ANNA_VM_NATIVE(anna_node_call_wrapper_i_init, 4)

/**
   This is the bulk of the each method
 */
static anna_vmstack_t *anna_node_call_wrapper_each_callback(anna_vmstack_t *stack, anna_object_t *me)
{    
    // Discard the output of the previous method call
    anna_vmstack_pop_object(stack);
    // Set up the param list. These are the values that aren't reallocated each lap
    anna_object_t **param = stack->top - 3;
    // Unwrap and name the params to make things more explicit
    anna_node_call_t *call = (anna_node_call_t *)anna_node_unwrap(param[0]);
    anna_object_t *body = param[1];
    int idx = anna_int_get(param[2]);
    size_t sz = call->child_count;
    
    // Are we done or do we need another lap?
    if(idx < sz)
    {
	// Set up params for the next lap of the each body function
	anna_object_t *o_param[] =
	    {
		param[2],
		anna_node_wrap(call->child[idx])
	    }
	;
	// Then update our internal lap counter
	param[2] = anna_int_create(idx+1);
	
	// Finally, roll the code point back a bit and push new arguments
	anna_vm_callback_reset(stack, body, 2, o_param);
    }
    else
    {
	// Oops, we're done. Drop our internal param list and push the correct output
	anna_vmstack_drop(stack, 4);
	anna_vmstack_push_object(stack, param[0]);
    }
    return stack;
}

static anna_vmstack_t *anna_node_call_wrapper_each(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t *body = anna_vmstack_pop_object(stack);
    anna_node_call_t *call = (anna_node_call_t *)anna_node_unwrap(anna_vmstack_pop_object(stack));
    anna_vmstack_pop_object(stack);
    size_t sz = call->child_count;

    if(sz > 0)
    {
	anna_object_t *callback_param[] = 
	    {
		anna_node_wrap((anna_node_t *)call),
		body,
		anna_int_one
	    }
	;
	
	anna_object_t *o_param[] =
	    {
		anna_int_zero,
		anna_node_wrap(call->child[0])
	    }
	;
	
	stack = anna_vm_callback_native(
	    stack,
	    anna_node_call_wrapper_each_callback, 3, callback_param,
	    body, 2, o_param
	    );
    }
    else
    {
	anna_vmstack_push_object(stack, anna_node_wrap((anna_node_t *)call));
    }
    
    return stack;
}

static inline anna_object_t *anna_node_call_wrapper_append_i(anna_object_t **param)
{
    anna_node_call_t *call = (anna_node_call_t *)anna_node_unwrap(param[0]);
    
    anna_object_t *list = param[1];
    
    if(list == null_object)
    {
	return param[0];
    }

    size_t i;    
    size_t size2 = anna_list_get_size(list);
    for(i=0; i<size2; i++)
    {
	anna_object_t *ch_obj = anna_list_get(list, i);
	anna_node_call_add_child(call, anna_node_unwrap(ch_obj));
    }
    
    return param[0];
}
ANNA_VM_NATIVE(anna_node_call_wrapper_append, 2)

static void anna_node_create_call_wrapper_type(anna_stack_template_t *stack)
{
    mid_t mmid;
    anna_function_t *fun;

    anna_type_copy(node_call_wrapper_type, node_wrapper_type);

    anna_type_t *argv[] = 
	{
	    node_call_wrapper_type,
	    node_wrapper_type,
	    node_wrapper_type,
	    anna_list_type_get(node_wrapper_type)
	}
    ;
    
    wchar_t *argn[] =
	{
	    L"this", L"source", L"func", L"param"
	}
    ;    

    anna_native_method_create(
	node_call_wrapper_type,
	-1,
	L"__init__",
	ANNA_FUNCTION_VARIADIC,
	&anna_node_call_wrapper_i_init, 
	object_type,
	4, argv, argn);

    anna_type_t *fun_type = anna_function_type_each_create(
	L"!CallIterFunction", int_type, node_wrapper_type);

    anna_type_t *e_argv[] = 
	{
	    node_call_wrapper_type,
	    fun_type
	}
    ;

    wchar_t *e_argn[]=
	{
	    L"this", L"block"
	}
    ;
    
    anna_native_method_create(
	node_call_wrapper_type, -1, L"__each__", 0, 
	&anna_node_call_wrapper_each, 
	node_call_wrapper_type,
	2, e_argv, e_argn);

    anna_native_property_create(
	node_call_wrapper_type,
	-1,
	L"count",
	int_type,
	&anna_node_call_wrapper_i_get_count, 
	0);

  
    anna_type_t *i_argv[] = 
	{
	    node_call_wrapper_type,
	    int_type,
	    node_wrapper_type
	}
    ;
    
    wchar_t *i_argn[] =
	{
	    L"this", L"index", L"value"
	}
    ;
    
    mmid = anna_native_method_create(
	node_call_wrapper_type,
	-1,
	L"__get__Int__",
	0, 
	&anna_node_call_wrapper_i_get_int, 
	node_wrapper_type,
	2, 
	i_argv, 
	i_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(node_call_wrapper_type, mmid));
    anna_function_alias_add(fun, L"__get__");

    mmid = anna_native_method_create(
	node_call_wrapper_type,
	-1,
	L"__set__Int__",
	0, 
	&anna_node_call_wrapper_i_set_int, 
	node_wrapper_type,
	3, 
	i_argv, 
	i_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(node_call_wrapper_type, mmid));
    anna_function_alias_add(fun, L"__set__");
    
    anna_native_property_create(
	node_call_wrapper_type,
	-1,
	L"function",
	node_wrapper_type,
	&anna_node_call_wrapper_i_get_function, 
	&anna_node_call_wrapper_i_set_function);
    
    anna_type_t *j_argv[] = 
	{
	    node_call_wrapper_type,
	    anna_list_type_get(node_wrapper_type)
	}
    ;
    
    wchar_t *j_argn[] =
	{
	    L"this", L"list"
	}
    ;
    
    mmid = anna_native_method_create(
	node_call_wrapper_type,
	-1,
	L"__join__List__",
	0, 
	&anna_node_call_wrapper_i_join_list, 
	node_call_wrapper_type,
	2, 
	j_argv, 
	j_argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(node_call_wrapper_type, mmid));
    anna_function_alias_add(fun, L"__join__");

    anna_native_method_create(
	node_call_wrapper_type, -1, L"__appendAssign__", 0, 
	&anna_node_call_wrapper_append, 
	node_call_wrapper_type,
	2, j_argv, j_argn);
    
}
