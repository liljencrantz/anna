//ROOT: src/lib/parser/parser.c

ANNA_VM_NATIVE(anna_node_null_wrapper_i_init, 2)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    if(anna_entry_null(param[1]))
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
		source?&source->location:0);
    }
    return param[0];
}

static anna_type_t *anna_node_create_null_type(anna_stack_template_t *stack)
{
    anna_type_t *node_null_type = node_null_literal_type;

    wchar_t *argn[] =
	{
	    L"this", L"source"
	}
    ;

    anna_type_t *argv[] = 
	{
	    node_null_type,
	    node_type,
	}
    ;
    
    anna_member_create_native_method(
	node_null_type,
	anna_mid_get(L"__init__"), 0,
	&anna_node_null_wrapper_i_init,
	node_null_type,
	2,
	argv,
	argn, 0, 0);

    anna_type_document(
	node_null_type,
	L"An AST node representing the null keyword (?).");

    return node_null_type;    
}

