
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
	    anna_string_get_count(param[2]),
	    anna_string_payload(param[2]));
    return param[0];
}

static void anna_node_create_string_literal_wrapper_type(anna_stack_frame_t *stack)
{
    node_string_literal_wrapper_type = anna_type_native_create(L"StringLiteral", stack);
    anna_type_copy(node_string_literal_wrapper_type, node_wrapper_type);
    anna_type_t *argv[] = 
	{
	    node_string_literal_wrapper_type,
	    node_wrapper_type,
	    string_type
	}
    ;
    
    wchar_t *argn[] =
	{
	    L"this", L"source", L"name"
	}
    ;
    
    anna_native_method_create(
	node_string_literal_wrapper_type,
	-1,
	L"__init__",
	0,
	&anna_node_string_literal_wrapper_i_init, 
	null_type,
	3, argv, argn);

    anna_native_property_create(
	node_string_literal_wrapper_type, -1, L"payload",
	string_type,
	&anna_node_string_literal_wrapper_i_get_name,
	0);
    
}

