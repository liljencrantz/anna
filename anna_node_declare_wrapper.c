
static inline anna_entry_t *anna_node_declare_wrapper_i_init_i(
    anna_entry_t **param)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_node_t *source = anna_node_unwrap(anna_as_obj(param[1]));


    *(anna_node_declare_t **)anna_entry_get_addr(this,ANNA_MID_NODE_PAYLOAD)=
	anna_node_create_declare(
	    source?&source->location:0,
	    anna_intern_or_free(anna_string_payload(anna_as_obj(param[2]))),
	    anna_node_unwrap(anna_as_obj(param[3])),
	    anna_node_unwrap(anna_as_obj(param[4])),
	    (anna_node_call_t *)anna_node_unwrap(anna_as_obj(param[5])),
	    !ANNA_VM_NULL(param[6]));
    return param[0];
}
ANNA_VM_NATIVE(anna_node_declare_wrapper_i_init, 7)

static anna_type_t *anna_node_create_declare_wrapper_type(
    anna_stack_template_t *stack)
{
    anna_type_t *type = anna_type_native_create(
	L"Declare", stack);
    
    wchar_t *argn[] =
	{
	    L"this", L"source", L"name", L"type", L"value", L"attributes", L"isConst"
	}
    ;
    
    anna_type_t *argv[] = 
	{
	    type,
	    node_wrapper_type,
	    string_type,
	    node_wrapper_type,
	    node_wrapper_type,
	    node_call_wrapper_type,
	    object_type,
	}
    ;
    
    anna_native_method_create(
	type,
	-1,
	L"__init__",
	0,
	&anna_node_declare_wrapper_i_init, 
	null_type,
	7, argv, argn);

    return type;
}
