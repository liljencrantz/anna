

static anna_node_t *anna_macro_if(anna_node_call_t *node,
				  anna_function_t *function, 
				  anna_node_list_t *parent)
{
/*
  wprintf(L"member_get on node at %d\n", node);
  anna_node_print((anna_node_t *)node);
  wprintf(L"\n");
*/
    CHECK_CHILD_COUNT(node,L"if macro", 2);
    CHECK_NODE_BLOCK(node->child[1]);
    
    anna_node_t *argv[] = {
        node->child[0], node->child[1], anna_node_null_create(&node->location)
    };
    
    return (anna_node_t *)
        anna_node_call_create(&node->location, 
			      (anna_node_t *)
			      anna_node_identifier_create(&node->location, 
							  L"__if__"),
			      3,
			      argv);
}

static anna_node_t *anna_macro_else(anna_node_call_t *node,
				    anna_function_t *function, 
				    anna_node_list_t *parent)
{
/*
  wprintf(L"member_get on node at %d\n", node);
  anna_node_print((anna_node_t *)node);
  wprintf(L"\n");
*/
    CHECK_CHILD_COUNT(node,L"else macro", 1);
    CHECK_NODE_BLOCK(node->child[0]);
    CHECK(parent->idx > 0,node, L"else with no matching if call");
    
    anna_node_call_t *parent_call = node_cast_call(parent->node);
    anna_node_t *prev = parent_call->child[parent->idx-1];
    /*
      anna_node_print(prev);   
      wprintf(L"\n");
    */
    anna_node_call_t *prev_call = node_cast_call(prev);
    anna_node_identifier_t *prev_call_name = node_cast_identifier(prev_call->function);
    
    CHECK(wcscmp(prev_call_name->name, L"__if__")==0,node, L"else with no matching if call");
    CHECK(prev_call->child_count == 3,prev_call, L"Bad if call");
    CHECK(prev_call->child[2]->node_type == ANNA_NODE_NULL,prev_call, L"Previous if statement already has an else clause");
    
    prev_call->child[2] = anna_node_prepare(node->child[0], function, parent);
    
    return (anna_node_t *)
	anna_node_null_create(&node->location);
   
}

static anna_object_t *anna_function_or(anna_object_t **param)
{
    return param[0] == null_object?anna_function_wrapped_invoke(param[1], 0, 0, 0):param[0];
}

static anna_node_t *anna_macro_or(anna_node_call_t *node, anna_function_t *function, anna_node_list_t *parent)
{
    CHECK_CHILD_COUNT(node,L"or operator", 2);
    
    anna_node_prepare_children(node, function, parent);

    anna_type_t * t1 = anna_node_get_return_type(node->child[0], function->stack_template);
    anna_type_t * t2 = anna_node_get_return_type(node->child[1], function->stack_template);
    
    CHECK(t1,node->child[0], L"Unknown type for first argument to operator or");
    CHECK(t2,node->child[1], L"Unknown type for decond argument to operator or");
    
    anna_type_t *return_type = anna_type_intersect(t1,t2);
    wchar_t *argn[]=
	{
	    L"condition1",
	    L"condition2"
	}
    ;
    
    anna_node_t *param[]=
	{
	    node->child[0],
	    (anna_node_t *)
	    anna_node_dummy_create(
		&node->location,
		anna_function_create(
		    L"!orConditionBlock", 0, 
		    anna_node_call_create(
			&node->location,
			(anna_node_t *)
			anna_node_identifier_create(
			    &node->location,
			    L"__block__"),
			1,
			&node->child[1]), 
		    t2, 0, 0, 0, 
		    function->stack_template,
		    function->return_pop_count+1)->wrapper,
		1)
	}
    ;
    anna_type_t *argv[]=
	{
	    t1,
	    anna_node_get_return_type(
		param[1], 
		function->stack_template)
	}
    ;
    
    return (anna_node_t *)
	anna_node_call_create(
	    &node->location, 
	    (anna_node_t *)
	    anna_node_dummy_create( 
		&node->location,
		anna_native_create(
		    L"!orAnonymous",
		    0,
		    (anna_native_t)anna_function_or,
		    return_type,
		    2,
		    argv,
		    argn)->wrapper,
		0),
	    2,
	    param);
}

static anna_object_t *anna_function_and(anna_object_t **param)
{
    return (param[0] == null_object)
	?null_object
	:anna_function_wrapped_invoke(param[1], 0, 0, 0);
}

static anna_node_t *anna_macro_and(anna_node_call_t *node, 
				   anna_function_t *function, 
				   anna_node_list_t *parent)
{
    CHECK_CHILD_COUNT(node,L"and operator", 2);
    
    anna_node_prepare_children(node, function, parent);
    
    anna_type_t * t1 = 
	anna_node_get_return_type(node->child[0], function->stack_template);
    anna_type_t * t2 = 
	anna_node_get_return_type(node->child[1], function->stack_template);
    
    CHECK(t1,node->child[0],L"Unknown type for first argument to operator and");
    CHECK(t2,node->child[1],L"Unknown type for second argument to operator and");	
    
    wchar_t *argn[]=
	{
	    L"condition1",
	    L"condition2"
	}
    ;
    anna_node_t *param[]=
	{
	    node->child[0],
	    (anna_node_t *)
	    anna_node_dummy_create(
		&node->location,
		anna_function_create(
		    L"!andConditionBlock", 0, 
		    anna_node_call_create(
			&node->location,
			(anna_node_t *)
			anna_node_identifier_create(
			    &node->location,
			    L"__block__"),
			1,
			&node->child[1]), 
		    t2, 0, 0, 0, 
		    function->stack_template, 
		    function->return_pop_count+1)->wrapper,
		1)
	}
    ;

    anna_type_t *argv[]=
	{
	    t1,
	    anna_node_get_return_type(
		param[1], 
		function->stack_template)
	}
    ;
    
    return (anna_node_t *)
	anna_node_call_create(
	    &node->location, 
	    (anna_node_t *)anna_node_dummy_create( 
		&node->location,
		anna_native_create(
		    L"!andAnonymous",
		    0,
		    (anna_native_t)anna_function_and,
		    t2,
		    2,
		    argv,
		    argn)->wrapper,
		0),
	    2,
	    param);
}

static anna_object_t *anna_function_while(anna_object_t **param)
{
    anna_object_t *result = null_object;
    while(anna_function_wrapped_invoke(param[0], 0, 0, 0) != null_object)
    {
	result = anna_function_wrapped_invoke(param[1], 0, 0, 0);
    }
    return result;
}

static anna_node_t *anna_macro_while(anna_node_call_t *node, 
				     anna_function_t *function,
				     anna_node_list_t *parent)
{
    CHECK_CHILD_COUNT(node,L"while macro", 2);
    CHECK_NODE_BLOCK(node->child[1]);
    
    anna_node_prepare_children(node, function, parent);
    
    anna_type_t *t2 = anna_node_get_return_type(node->child[1], function->stack_template);    
    CHECK(t2,node->child[1], L"Unknown type for second argument to while");	
    
    anna_node_t *condition = 
	(anna_node_t *)
	anna_node_dummy_create(
	    &node->location,
	    anna_function_create(
		L"!andConditionBlock", 0, 
		anna_node_call_create(
		    &node->location,
		    (anna_node_t *)anna_node_identifier_create(
			&node->location,
			L"__block__"),
		    1,
		    &node->child[0]), 
		t2, 0, 0, 0, 
		function->stack_template,
		function->return_pop_count+1)->wrapper,
	    1);
    
    wchar_t *argn[]=
	{
	    L"condition",
	    L"body"
	}
    ;
    
    anna_node_t *param[]=
	{
	    condition,
	    node->child[1]
	}
    ;
    
    anna_type_t *argv[]=
	{
	    anna_node_get_return_type(param[0], function->stack_template),
	    t2
	}
    ;
    /*
      FIXME: I think the return values are all wrong here, need to
      make sure we're rturning the function result, not the functio
      type itself...
    */
    return (anna_node_t *)
	anna_node_call_create(
	    &node->location,
	    (anna_node_t *)anna_node_dummy_create( 
		&node->location,
		anna_native_create(
		    L"!whileAnonymous",
		    0,
		    (anna_native_t)anna_function_while,
		    t2,
		    2,
		    argv,
		    argn)->wrapper,
		0),
	    2,
	    param);
}
