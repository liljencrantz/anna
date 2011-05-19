
static inline anna_entry_t *anna_node_null_wrapper_i_init_i(anna_entry_t **param)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    if(ANNA_VM_NULL(param[1]))
    {
	*(anna_node_t **)anna_entry_get_addr(this,ANNA_MID_NODE_PAYLOAD)=
	    anna_node_create_null(
		0);
    }
    else
    {
	anna_node_t *source = anna_node_unwrap(anna_as_obj(param[1]));
	*(anna_node_t **)anna_entry_get_addr(this,ANNA_MID_NODE_PAYLOAD)=
	    anna_node_create_null(
		&source->location);
    }
    return param[0];
}
ANNA_VM_NATIVE(anna_node_null_wrapper_i_init, 2)

static anna_type_t *anna_node_create_null_wrapper_type(anna_stack_template_t *stack)
{
    anna_type_t *node_null_wrapper_type = anna_type_native_create(L"Null", stack);
    anna_type_copy(node_null_wrapper_type, node_wrapper_type);

    wchar_t *argn[] =
	{
	    L"this", L"source"
	}
    ;

    anna_type_t *argv[] = 
	{
	    node_null_wrapper_type,
	    node_wrapper_type,
	}
    ;
    
    anna_native_method_create(
	node_null_wrapper_type,
	-1,
	L"__init__",
	0,
	&anna_node_null_wrapper_i_init, 
	node_null_wrapper_type,
	2, argv, argn);
    return node_null_wrapper_type;
    
}

