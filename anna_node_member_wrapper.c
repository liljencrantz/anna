static inline anna_entry_t *anna_node_member_wrapper_i_get_name_i(anna_entry_t **param)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_member_access_t *node = (anna_node_member_access_t *)anna_node_unwrap(this);
    wchar_t *name = anna_mid_get_reverse(node->mid);
    return anna_from_obj(anna_string_create(wcslen(name), name));
}
ANNA_VM_NATIVE(anna_node_member_wrapper_i_get_name, 1)

static inline anna_entry_t *anna_node_member_get_wrapper_i_init_i(
    anna_entry_t **param)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_t *source = anna_node_unwrap(anna_as_obj(param[1]));
    anna_node_t *object = anna_node_unwrap(anna_as_obj(param[2]));
    wchar_t *name = anna_string_payload(anna_as_obj(param[3]));

    *(anna_node_member_access_t **)anna_entry_get_addr(this,ANNA_MID_NODE_PAYLOAD)=
	anna_node_create_member_get(
	    source?&source->location:0,
	    object,
	    anna_mid_get(name));
    return param[0];
}
ANNA_VM_NATIVE(anna_node_member_get_wrapper_i_init, 4)

static inline anna_entry_t *anna_node_member_set_wrapper_i_init_i(
    anna_entry_t **param)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_t *source = anna_node_unwrap(anna_as_obj(param[1]));
    anna_node_t *object = anna_node_unwrap(anna_as_obj(param[2]));
    wchar_t *name = anna_string_payload(anna_as_obj(param[3]));
    anna_node_t *value = anna_node_unwrap(anna_as_obj(param[4]));

    *(anna_node_member_access_t **)anna_entry_get_addr(this,ANNA_MID_NODE_PAYLOAD)=
	anna_node_create_member_set(
	    source?&source->location:0,
	    object,
	    anna_mid_get(name),
	    value);
    return param[0];
}
ANNA_VM_NATIVE(anna_node_member_set_wrapper_i_init, 5)

static anna_type_t *anna_node_create_member_wrapper_type(anna_stack_template_t *stack, int node)
{
    anna_type_t *node_member_wrapper_type = anna_type_native_create(node==ANNA_NODE_MEMBER_GET?L"MemberGet":L"MemberSet", stack);
    
    wchar_t *argn[] =
	{
	    L"this", L"source", L"object", L"name", L"value"
	}
    ;
    
    anna_type_t *argv[] = 
	{
	    node_member_wrapper_type,
	    node_wrapper_type,
	    node_wrapper_type,
	    string_type,
	    node_wrapper_type
	}
    ;
    if(node == ANNA_NODE_MEMBER_GET)
    {
	anna_native_method_create(
	    node_member_wrapper_type,
	    -1,
	    L"__init__",
	    0,
	    &anna_node_member_get_wrapper_i_init, 
	    null_type,
	    4, argv, argn);
    }
    else
    {
	anna_native_method_create(
	    node_member_wrapper_type,
	    -1,
	    L"__init__",
	    0,
	    &anna_node_member_set_wrapper_i_init, 
	    null_type,
	    5, argv, argn);	
    }
    
    anna_native_property_create(
	node_member_wrapper_type, -1, L"name",
	string_type,
	&anna_node_member_wrapper_i_get_name, 
	0);

    return node_member_wrapper_type;
}
