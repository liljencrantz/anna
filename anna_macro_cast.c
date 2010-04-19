

static anna_node_t *anna_macro_cast(
    anna_node_call_t *node, 
    anna_function_t *function, 
    anna_node_list_t *parent)
{
    return (anna_node_t *)anna_node_call_create(
	&node->location,
	(anna_node_t *)anna_node_identifier_create(
	    &node->function->location,
	    L"List"), 
	node->child_count,
	node->child);
}


static anna_object_t *anna_function_as(anna_object_t **param)
{
    anna_type_t *type = anna_type_unwrap(param[1]);
/*    wprintf(L"Check if object of type %ls abides to type %ls\n",
      param[0]->type->name, type->name);*/
    return anna_abides(param[0]->type, type)?param[0]:null_object;
}

static anna_node_t *anna_macro_as(
    anna_node_call_t *node, 
    anna_function_t *function, 
    anna_node_list_t *parent)
{
    anna_node_prepare_children(node, function, parent);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);

    anna_type_t *as_argv[]={object_type, type_type};
    wchar_t *as_argn[]={L"object", L"type"};    

    anna_node_identifier_t *type_name = 
	(anna_node_identifier_t *)node->child[1];
    anna_object_t *obj = anna_stack_get_str(function->stack_template, type_name->name);
    CHECK(obj, node->child[1], L"Unknown type: %ls", type_name->name);
    anna_type_t *type = anna_type_unwrap(obj);
    CHECK(type, node->child[1], L"Not a type: %ls", type_name->name);

    anna_node_t *param[]=
	{
	    node->child[0],
	    node->child[1]
	}
    ;
    
    
    return (anna_node_t *)
	anna_node_call_create(
	    &node->location,
	    (anna_node_t *)anna_node_dummy_create( 
		&node->location,
		anna_function_wrap(
		    anna_native_create(
			L"!asAnonymous",
			0,
			(anna_native_t)anna_function_as,
			type,
			2,
			as_argv,
			as_argn,
			0)),
		0),
	    2,
	    param);
}
