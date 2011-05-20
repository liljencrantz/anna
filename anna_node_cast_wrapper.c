static inline anna_entry_t *anna_node_cast_wrapper_i_get_value_i(
    anna_entry_t **param)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(this);
    return anna_from_obj(anna_node_wrap(node->child[0]));
}
ANNA_VM_NATIVE(anna_node_cast_wrapper_i_get_value, 1)

static inline anna_entry_t *anna_node_cast_wrapper_i_get_as_i(
    anna_entry_t **param)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_call_t *node = (anna_node_call_t *)anna_node_unwrap(this);
    return anna_from_obj(anna_node_wrap(node->child[1]));
}
ANNA_VM_NATIVE(anna_node_cast_wrapper_i_get_as, 1)

static inline anna_entry_t *anna_node_cast_wrapper_i_init_i(
    anna_entry_t **param)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_t *source = anna_node_unwrap(anna_as_obj(param[1]));
    *(anna_node_call_t **)anna_entry_get_addr(this,ANNA_MID_NODE_PAYLOAD)=
	anna_node_create_call2(
	    &source->location,
	    anna_node_create_null(&source->location),
	    anna_node_unwrap(anna_as_obj(param[2])),
	    anna_node_unwrap(anna_as_obj(param[3])));
    (*(anna_node_call_t **)anna_entry_get_addr(this,ANNA_MID_NODE_PAYLOAD))->node_type = ANNA_NODE_CAST;
    return param[0];
}
ANNA_VM_NATIVE(anna_node_cast_wrapper_i_init, 4)

static anna_type_t *anna_node_create_cast_wrapper_type(
    anna_stack_template_t *stack)
{
    anna_type_t *type = anna_type_native_create(
	L"Cast", stack);
    
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
	&anna_node_cast_wrapper_i_init, 
	null_type,
	4, argv, argn);
    
    anna_native_property_create(
	type, -1, L"value",
	node_wrapper_type,
	&anna_node_cast_wrapper_i_get_value, 
	0);

    anna_native_property_create(
	type, -1, L"as",
	node_wrapper_type,
	&anna_node_cast_wrapper_i_get_as, 
	0);

    return type;
}
