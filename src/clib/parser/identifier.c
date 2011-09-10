ANNA_VM_NATIVE(anna_node_identifier_wrapper_i_get_name, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_identifier_t *node = (anna_node_identifier_t *)anna_node_unwrap(this);
    return anna_from_obj(anna_string_create(wcslen(node->name), node->name));
}

ANNA_VM_NATIVE(anna_node_identifier_wrapper_i_init, 3)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_t *source = anna_node_unwrap(anna_as_obj(param[1]));
    *(anna_node_identifier_t **)anna_entry_get_addr(this,ANNA_MID_NODE_PAYLOAD)=
	anna_node_create_identifier(
	    source?&source->location:0,
	    anna_intern_or_free(anna_string_payload(anna_as_obj(param[2]))));
    return param[0];
}

ANNA_VM_NATIVE(anna_node_mapping_identifier_wrapper_i_init, 3)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_t *source = anna_node_unwrap(anna_as_obj(param[1]));
    anna_node_identifier_t *n = anna_node_create_identifier(
	source?&source->location:0,
	anna_intern_or_free(anna_string_payload(anna_as_obj(param[2]))));
    n->node_type = ANNA_NODE_INTERNAL_IDENTIFIER;
    *(anna_node_identifier_t **)anna_entry_get_addr(this,ANNA_MID_NODE_PAYLOAD)=
	n;
    
    return param[0];
}

static void anna_node_create_identifier_type(anna_stack_template_t *stack, anna_type_t *type, int mapping)
{
    wchar_t *argn[] =
	{
	    L"this", L"source", L"name"
	}
    ;

    anna_type_t *argv[] = 
	{
	    type,
	    node_type,
	    string_type
	}
    ;
    
    anna_member_create_native_method(
	type, anna_mid_get(L"__init__"), 0,
	mapping ? &anna_node_mapping_identifier_wrapper_i_init : &anna_node_identifier_wrapper_i_init,
	null_type,
	3,
	argv,
	argn, 0, 0);

    anna_member_create_native_property(
	type, anna_mid_get(L"name"), string_type,
	&anna_node_identifier_wrapper_i_get_name, 0,
	L"The name of this identifier.");
}

