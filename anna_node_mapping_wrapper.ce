static inline anna_entry_t *anna_node_mapping_wrapper_i_get_from_i(
    anna_entry_t **param)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_cond_t *node = (anna_node_cond_t *)anna_node_unwrap(this);
    return anna_from_obj(anna_node_wrap(node->arg1));
}
ANNA_VM_NATIVE(anna_node_mapping_wrapper_i_get_from, 1)

static inline anna_entry_t *anna_node_mapping_wrapper_i_get_to_i(
    anna_entry_t **param)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_cond_t *node = (anna_node_cond_t *)anna_node_unwrap(this);
    return anna_from_obj(anna_node_wrap(node->arg2));
}
ANNA_VM_NATIVE(anna_node_mapping_wrapper_i_get_to, 1)

static inline anna_entry_t *anna_node_mapping_wrapper_i_init_i(
    anna_entry_t **param)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_t *source = anna_node_unwrap(anna_as_obj(param[1]));
    *(anna_node_cond_t **)anna_entry_get_addr(this,ANNA_MID_NODE_PAYLOAD)=
	anna_node_create_mapping(
	    source?&source->location:0,
	    anna_node_unwrap(anna_as_obj(param[2])),
	    anna_node_unwrap(anna_as_obj(param[3])));
    return param[0];
}
ANNA_VM_NATIVE(anna_node_mapping_wrapper_i_init, 4)

static anna_type_t *anna_node_create_mapping_wrapper_type(
    anna_stack_template_t *stack)
{
    anna_type_t *type = anna_type_native_create(
	L"Mapping", stack);
    
    wchar_t *argn[] =
	{
	    L"this", L"source", L"from", L"to"
	}
    ;
    
    anna_type_t *argv[] = 
	{
	    type,
	    node_wrapper_type,
	    node_wrapper_type,
	    node_wrapper_type,
	}
    ;
    
    anna_native_method_create(
	type,
	-1,
	L"__init__",
	0,
	&anna_node_mapping_wrapper_i_init, 
	null_type,
	4, argv, argn);
    
    anna_native_property_create(
	type, -1, L"from",
	node_wrapper_type,
	&anna_node_mapping_wrapper_i_get_from, 
	0);

    anna_native_property_create(
	type, -1, L"to",
	node_wrapper_type,
	&anna_node_mapping_wrapper_i_get_to, 
	0);

    return type;
}
