
static anna_object_t *anna_node_int_literal_wrapper_i_get_payload(anna_object_t **param)
{
    anna_node_int_literal_t *node = (anna_node_int_literal_t *)anna_node_unwrap(param[0]);
    return anna_int_create(node->payload);
}

static anna_object_t *anna_node_int_literal_wrapper_i_init(anna_object_t **param)
{
    assert(param[0] != null_object);
    assert(param[1] != null_object);
    assert(param[2] != null_object);
    anna_node_t *source = anna_node_unwrap(param[1]);
    *(anna_node_t **)anna_member_addr_get_mid(
	param[0],
	ANNA_MID_NODE_PAYLOAD)=
	anna_node_int_literal_create(
	    &source->location,
	    anna_int_get(param[2]));
    return param[0];
}

void anna_node_int_literal_wrapper_type_create(anna_stack_frame_t *stack)
{
    anna_node_t *argv[] = 
	{
	    (anna_node_t *)anna_node_identifier_create(0, L"Identifier"),
	    (anna_node_t *)anna_node_identifier_create(0, L"Node"),
	    (anna_node_t *)anna_node_identifier_create(0, L"Int"),
	}
    ;
    
    wchar_t *argn[] =
	{
	    L"this", L"source", L"payload"
	}
    ;

    node_int_literal_wrapper_type = anna_type_native_create(L"IntLiteral", stack);
    anna_type_native_parent(node_int_literal_wrapper_type, L"Node");
    
    anna_node_call_t *definition = anna_type_definition_get(node_int_literal_wrapper_type);
    
    anna_native_method_add_node(
	definition,
	-1,
	L"__init__",
	0,
	(anna_native_t)&anna_node_int_literal_wrapper_i_init, 
	(anna_node_t *)anna_node_identifier_create(0, L"Null") , 
	3, argv, argn);
    
    anna_native_method_add_node(
	definition, -1, L"getPayload", 0, 
	(anna_native_t)&anna_node_int_literal_wrapper_i_get_payload, 
	(anna_node_t *)anna_node_identifier_create(0, L"Int"), 
	1, argv, argn);
    
    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_property_create(
	    0,
	    L"payload",
	    (anna_node_t *)anna_node_identifier_create(0, L"Int") , 
	    L"getPayload", 0));
    
}
