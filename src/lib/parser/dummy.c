//ROOT: src/lib/parser/parser.c

ANNA_VM_NATIVE(anna_node_dummy_wrapper_i_get_payload, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_dummy_t *node = (anna_node_dummy_t *)anna_node_unwrap(this);
    return anna_from_obj(node->payload);
}

ANNA_VM_NATIVE(anna_node_dummy_wrapper_i_init, 3)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_t *source = anna_node_unwrap(anna_as_obj(param[1]));
    *(anna_node_dummy_t **)anna_entry_get_addr(this,ANNA_MID_NODE_PAYLOAD)=
	anna_node_create_dummy(
	    source?&source->location:0,
	    anna_as_obj(param[2]));
    return param[0];
}

static anna_type_t *anna_node_create_dummy_type(anna_stack_template_t *stack)
{
    anna_type_t *node_dummy_type = anna_type_create(L"Dummy", 0);
    
    wchar_t *argn[] =
	{
	    L"this", L"source", L"payload"
	}
    ;
    
    anna_type_t *argv[] = 
	{
	    node_dummy_type,
	    node_type,
	    any_type
	}
    ;
    
    anna_member_create_native_method(
	node_dummy_type,
	anna_mid_get(L"__init__"), 0,
	&anna_node_dummy_wrapper_i_init,
	null_type,
	3,
	argv,
	argn, 0, 0);
    
    anna_member_create_native_property(
	node_dummy_type,
	anna_mid_get(L"payload"), any_type,
	&anna_node_dummy_wrapper_i_get_payload, 0, 
	L"The payload of this node.");

    return node_dummy_type;
}
