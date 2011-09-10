
ANNA_VM_NATIVE(anna_node_string_literal_wrapper_i_get_name, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_string_literal_t *node = (anna_node_string_literal_t *)anna_node_unwrap(this);
    return anna_from_obj(anna_string_create(node->payload_size, node->payload));
}

ANNA_VM_NATIVE(anna_node_string_literal_wrapper_i_init, 3)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_t *source = anna_node_unwrap(anna_as_obj(param[1]));
    *(anna_node_t **)anna_entry_get_addr(
	this,ANNA_MID_NODE_PAYLOAD)=
	(anna_node_t *)anna_node_create_string_literal(
	    source?&source->location:0,
	    anna_string_get_count(anna_as_obj(param[2])),
	    anna_intern_or_free(anna_string_payload(anna_as_obj(param[2]))));
    return param[0];
}

static anna_type_t *anna_node_create_string_literal_type(anna_stack_template_t *stack)
{
    anna_type_t *node_string_literal_type = anna_type_native_create(L"StringLiteral", stack);
    anna_type_t *argv[] = 
	{
	    node_string_literal_type,
	    node_type,
	    string_type
	}
    ;
    
    wchar_t *argn[] =
	{
	    L"this", L"source", L"name"
	}
    ;
    
    anna_member_create_native_method(
	node_string_literal_type,
	anna_mid_get(L"__init__"), 0,
	&anna_node_string_literal_wrapper_i_init,
	null_type,
	3,
	argv,
	argn, 0, 0);

    anna_member_create_native_property(
	node_string_literal_type,
	anna_mid_get(L"payload"), string_type,
	&anna_node_string_literal_wrapper_i_get_name,
	0,
	L"The payload of this node");
    return node_string_literal_type;
    
}

