
static anna_object_t *anna_node_string_literal_wrapper_i_get_name(anna_object_t **param)
{
    anna_node_string_literal_t *node = (anna_node_string_literal_t *)anna_node_unwrap(param[0]);
    return anna_string_create(node->payload_size, node->payload);
}

static anna_object_t *anna_node_string_literal_wrapper_i_init(anna_object_t **param)
{
    assert(param[0] != null_object);
    assert(param[1] != null_object);
    assert(param[2] != null_object);
    anna_node_t *source = anna_node_unwrap(param[1]);
    *(anna_node_t **)anna_member_addr_get_mid(
	param[0],ANNA_MID_NODE_PAYLOAD)=
	(anna_node_t *)anna_node_create_string_literal(
	    &source->location,
	    anna_string_count(param[2]),
	    anna_string_payload(param[2]));
    return param[0];
}

void anna_node_create_string_literal_wrapper_type(anna_stack_frame_t *stack)
{
    anna_node_t *argv[] = 
	{
	    (anna_node_t *)anna_node_create_identifier(0, L"StringLiteral"),
	    (anna_node_t *)anna_node_create_identifier(0, L"Node"),
	    (anna_node_t *)anna_node_create_identifier(0, L"String"),
	}
    ;
    
    wchar_t *argn[] =
	{
	    L"this", L"source", L"name"
	}
    ;

    node_string_literal_wrapper_type = anna_type_native_create(L"StringLiteral", stack);
    anna_type_native_parent(node_string_literal_wrapper_type, L"Node");
    
    anna_node_call_t *definition = anna_type_definition_get(node_string_literal_wrapper_type);
    
    anna_native_method_add_node(
	definition,
	-1,
	L"__init__",
	0,
	(anna_native_t)&anna_node_string_literal_wrapper_i_init, 
	(anna_node_t *)anna_node_create_identifier(0, L"Null") , 
	3, argv, argn);


    anna_native_method_add_node(
	definition, -1, L"getPayload", 0, 
	(anna_native_t)&anna_node_string_literal_wrapper_i_get_name, 
	(anna_node_t *)anna_node_create_identifier(0, L"String"), 
	1, argv, argn);
    
    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_create_property(
	    0,
	    L"payload",
	    (anna_node_t *)anna_node_create_identifier(0, L"String") , 
	    L"getPayload", 0));
    
}

