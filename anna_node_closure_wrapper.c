ANNA_NATIVE(anna_node_closure_wrapper_i_get_payload, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_closure_t *node = (anna_node_closure_t *)anna_node_unwrap(this);
    return anna_from_obj(anna_function_wrap(node->payload));
}

ANNA_NATIVE(anna_node_closure_wrapper_i_init, 3)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_t *source = anna_node_unwrap(anna_as_obj(param[1]));
    *(anna_node_closure_t **)anna_entry_get_addr(this,ANNA_MID_NODE_PAYLOAD)=
	anna_node_create_closure(
	    source?&source->location:0,
	    anna_function_unwrap(anna_as_obj(param[2])));
    return param[0];
}

static anna_type_t *anna_node_create_closure_wrapper_type(
    anna_stack_template_t *stack)
{
    anna_type_t *type = anna_type_native_create(
	L"Closure", stack);
    
    wchar_t *argn[] =
	{
	    L"this", L"source", L"payload"
	}
    ;
    
    anna_type_t *argv[] = 
	{
	    type,
	    node_wrapper_type,
	    function_type_base
	}
    ;
    
    anna_member_create_native_method(
	type, anna_mid_get(L"__init__"), 0,
	&anna_node_closure_wrapper_i_init,
	null_type,
	3,
	argv,
	argn);
    
    anna_member_create_native_property(
	type, anna_mid_get(L"payload"),
	function_type_base,
	&anna_node_closure_wrapper_i_get_payload,
	0);

    return type;
}
