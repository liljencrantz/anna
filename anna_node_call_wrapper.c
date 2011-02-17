

static anna_object_t *anna_node_call_wrapper_i_get_count(anna_object_t **param)
{
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(param[0]);
    return anna_int_create(node->child_count);
}

static anna_object_t *anna_node_call_wrapper_i_get_int(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(param[0]);
    int idx = anna_int_get(param[1]);
    if(idx < 0 || idx >= node->child_count)
	return null_object;
    return anna_node_wrap(node->child[idx]);
}

static anna_object_t *anna_node_call_wrapper_i_set_int(anna_object_t **param)
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

static anna_object_t *anna_node_call_wrapper_i_get_function(anna_object_t **param)
{
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(param[0]);
    return anna_node_wrap(node->function);
}

static anna_object_t *anna_node_call_wrapper_i_set_function(anna_object_t **param)
{
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(param[0]);
    if(param[1]==null_object)
	return null_object;
    node->function = anna_node_unwrap(param[1]);
    return param[1];
    
}

static anna_object_t *anna_node_call_wrapper_i_join_list(anna_object_t **param)
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

static anna_object_t *anna_node_call_wrapper_i_init(anna_object_t **param)
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

static anna_object_t *anna_node_call_wrapper_i_each(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;

    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(param[0]);
    size_t i;

    anna_object_t *body_object;
    body_object=param[1];

    anna_function_t **function_ptr = (anna_function_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    anna_stack_frame_t **stack_ptr = (anna_stack_frame_t **)anna_member_addr_get_mid(body_object, ANNA_MID_FUNCTION_WRAPPER_STACK);
    anna_stack_frame_t *stack = stack_ptr?*stack_ptr:0;
    assert(function_ptr);
/*
  wprintf(L"each loop got function %ls\n", (*function_ptr)->name);
  wprintf(L"with param %ls\n", (*function_ptr)->input_name[0]);
*/  
    anna_object_t *o_param[2];
    for(i=0;i<node->child_count;i++)
    {
	/*
	  wprintf(L"Run the following code:\n");
	  anna_node_print((*function_ptr)->body);
	  wprintf(L"\n");
	*/
	o_param[0] = anna_int_create(i);
	o_param[1] = anna_node_wrap(node->child[i]);
	anna_function_invoke_values(*function_ptr, 0, o_param, stack);
    }
    return param[0];
}

void anna_node_create_call_wrapper_type(anna_stack_frame_t *stack)
{
/*
    anna_node_t *list_template_param[] = 
	{
	    (anna_node_t *)anna_node_create_identifier(0, L"Node")
	}
    ;
    
    anna_node_t *node_list_type = anna_node_create_templated_type(
	0, 
	(anna_node_t *)anna_node_create_identifier(0, L"List"),
	1,
	list_template_param);


    anna_node_t *j_argv[] = 
	{
	    (anna_node_t *)anna_node_create_identifier(0, L"Call"),
	    anna_node_create_simple_template(
		0,
		L"List",
		L"Node"),
	}
    ;
    
    wchar_t *j_argn[] =
	{
	    L"this", L"list"
	}
    ;
*/    
    node_call_wrapper_type = anna_type_native_create(L"Call", stack);
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
	L"!StringIterFunction", node_wrapper_type);

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
	&anna_node_call_wrapper_i_each, 
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
    
    anna_native_method_create(
	node_call_wrapper_type,
	-1,
	L"__get__Int__",
	0, 
	&anna_node_call_wrapper_i_get_int, 
	node_wrapper_type,
	2, 
	i_argv, 
	i_argn);

    anna_native_method_create(
	node_call_wrapper_type,
	-1,
	L"__set__Int__",
	0, 
	&anna_node_call_wrapper_i_set_int, 
	node_wrapper_type,
	3, 
	i_argv, 
	i_argn);

/*    
    anna_native_method_create(
	node_call_wrapper_type,
	-1,
	L"__join__List__",
	0, 
	(anna_native_t)&anna_node_call_wrapper_i_join_list, 
	(anna_node_t *)anna_node_create_identifier(0, L"Call") , 
	2, 
	j_argv, 
	j_argn);
    
    anna_node_call_add_child(
	node_call_wrapper_type,
	(anna_node_t *)anna_node_create_property(
	    0,
	    L"count",
	    (anna_node_t *)anna_node_create_identifier(0, L"Int") , 
	    L"getCount",
	    0));
	
    anna_native_method_create(
	node_call_wrapper_type, -1, L"setFunction", 0, 
	(anna_native_t)&anna_node_call_wrapper_i_set_function, 
	(anna_node_t *)anna_node_create_identifier(0, L"Node"), 
	2, argv, argn);
    
    anna_native_method_create(
	node_call_wrapper_type, -1, L"getFunction", 0, 
	(anna_native_t)&anna_node_call_wrapper_i_get_function, 
	(anna_node_t *)anna_node_create_identifier(0, L"Node"), 
	1, argv, argn);
    
    anna_node_call_add_child(
	node_call_wrapper_type,
	(anna_node_t *)anna_node_create_property(
	    0,
	    L"func",
	    (anna_node_t *)anna_node_create_identifier(0, L"Node") , 
	    L"getFunction",
	    L"setFunction"));
    
*/    
}
