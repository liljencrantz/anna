
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
	(anna_node_t *)anna_node_create_int_literal(
	    &source->location,
	    anna_int_get(param[2]));
    return param[0];
}

static void anna_node_create_int_literal_wrapper_type(anna_stack_template_t *stack)
{
    node_int_literal_wrapper_type = anna_type_native_create(L"IntLiteral", stack);
    anna_type_copy(node_int_literal_wrapper_type, node_wrapper_type);

    anna_type_t *argv[] = 
	{
	    node_int_literal_wrapper_type,
	    node_wrapper_type,
	    int_type
	}
    ;
    
    wchar_t *argn[] =
	{
	    L"this", L"source", L"payload"
	}
    ;

    anna_native_method_create(
	node_int_literal_wrapper_type,
	-1,
	L"__init__",
	0,
	&anna_node_int_literal_wrapper_i_init, 
	null_type,
	3, argv, argn);
    
    anna_native_property_create(
	node_int_literal_wrapper_type, -1, L"payload",
	int_type,
	&anna_node_int_literal_wrapper_i_get_payload,
	0);
    
}
