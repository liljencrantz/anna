
ANNA_VM_NATIVE(anna_node_float_literal_wrapper_i_get_payload, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_float_literal_t *node = (anna_node_float_literal_t *)anna_node_unwrap(this);
    return anna_from_float(node->payload);
}

ANNA_VM_NATIVE(anna_node_float_literal_wrapper_i_init, 3)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_t *source = anna_node_unwrap(anna_as_obj(param[1]));
    *(anna_node_float_literal_t **)anna_entry_get_addr(this,ANNA_MID_NODE_PAYLOAD)=
	anna_node_create_float_literal(
	    source?&source->location:0,
	    anna_as_float(param[2]));
    return param[0];
}

static anna_type_t *anna_node_create_float_literal_type(anna_stack_template_t *stack)
{
    anna_type_t *type = anna_type_native_create(L"FloatLiteral", stack);
    
    wchar_t *argn[] =
	{
	    L"this", L"source", L"payload"
	}
    ;
    
    anna_type_t *argv[] = 
	{
	    type,
	    node_type,
	    float_type
	}
    ;
    
    anna_member_create_native_method(
	type, anna_mid_get(L"__init__"), 0,
	&anna_node_float_literal_wrapper_i_init,
	null_type,
	3,
	argv,
	argn);
    
    anna_member_create_native_property(
	type, anna_mid_get(L"payload"), float_type,
	&anna_node_float_literal_wrapper_i_get_payload,
	0,
	L"The payload of this node");
    return type;
}
