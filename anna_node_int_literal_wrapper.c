
static inline anna_entry_t *anna_node_int_literal_wrapper_i_get_payload_i(
    anna_entry_t **param)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_int_literal_t *node = (anna_node_int_literal_t *)anna_node_unwrap(this);
    return anna_from_obj(anna_int_create_mp(node->payload));
}
ANNA_VM_NATIVE(anna_node_int_literal_wrapper_i_get_payload, 1)

static inline anna_entry_t *anna_node_int_literal_wrapper_i_init_i(
    anna_entry_t **param)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_t *source = anna_node_unwrap(anna_as_obj(param[1]));
    *(anna_node_t **)anna_entry_get_addr(
	this,
	ANNA_MID_NODE_PAYLOAD)=
	(anna_node_t *)anna_node_create_int_literal(
	    &source->location,
	    *anna_int_unwrap(anna_as_obj(param[2])));
    return param[0];
}
ANNA_VM_NATIVE(anna_node_int_literal_wrapper_i_init, 3)

static anna_type_t *anna_node_create_int_literal_wrapper_type(
    anna_stack_template_t *stack)
{
    anna_type_t *type = anna_type_native_create(L"IntLiteral", stack);

    anna_type_t *argv[] = 
	{
	    type,
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
	type,
	-1,
	L"__init__",
	0,
	&anna_node_int_literal_wrapper_i_init, 
	null_type,
	3, argv, argn);
    
    anna_native_property_create(
	type, -1, L"payload",
	int_type,
	&anna_node_int_literal_wrapper_i_get_payload,
	0);
    return type;
}
