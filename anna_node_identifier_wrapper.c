static anna_object_t *anna_node_identifier_wrapper_i_get_name(anna_object_t **param)
{
    anna_node_identifier_t *node = (anna_node_identifier_t *)anna_node_unwrap(param[0]);
    return anna_string_create(wcslen(node->name), node->name);
}

static anna_object_t *anna_node_identifier_wrapper_i_init(anna_object_t **param)
{
    assert(param[0] != null_object);
    assert(param[1] != null_object);
    assert(param[2] != null_object);
    anna_node_t *source = anna_node_unwrap(param[1]);
    *(anna_node_identifier_t **)anna_member_addr_get_mid(param[0],ANNA_MID_NODE_PAYLOAD)=
	anna_node_create_identifier(
	    &source->location,
	    wcsdup(anna_string_payload(param[2])));
    return param[0];
}

static void anna_node_create_identifier_wrapper_type(anna_stack_template_t *stack)
{
    node_identifier_wrapper_type = anna_type_native_create(L"Identifier", stack);
    anna_type_copy(node_identifier_wrapper_type, node_wrapper_type);

    wchar_t *argn[] =
	{
	    L"this", L"source", L"name"
	}
    ;

    anna_type_t *argv[] = 
	{
	    node_identifier_wrapper_type,
	    node_wrapper_type,
	    string_type
	}
    ;
    
    anna_native_method_create(
	node_identifier_wrapper_type,
	-1,
	L"__init__",
	0,
	&anna_node_identifier_wrapper_i_init, 
	null_type,
	3, argv, argn);

    anna_native_property_create(
	node_identifier_wrapper_type, -1, L"name",
	string_type,
	&anna_node_identifier_wrapper_i_get_name, 
	0);
}

