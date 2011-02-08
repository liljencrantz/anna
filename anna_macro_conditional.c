
static anna_node_t *anna_macro_or(struct anna_node_call *node)
{
    CHECK_CHILD_COUNT(node,L"if macro", 2);
    return (anna_node_t *)
	anna_node_create_cond(
	    &node->location, 
	    ANNA_NODE_OR,
	    node->child[0],
	    node->child[1]);
}


static anna_node_t *anna_macro_and(struct anna_node_call *node)
{
    CHECK_CHILD_COUNT(node,L"if macro", 2);
    return (anna_node_t *)
	anna_node_create_cond(
	    &node->location, 
	    ANNA_NODE_AND,
	    node->child[0],
	    node->child[1]);
}

static anna_node_t *anna_macro_if(anna_node_call_t *node)
{

    CHECK_CHILD_COUNT(node,L"if macro", 3);
    CHECK_NODE_BLOCK(node->child[1]);
    CHECK_NODE_BLOCK(node->child[2]);
    
    return (anna_node_t *)
        anna_node_create_if(
	    &node->location, 
	    node->child[0],
	    (anna_node_call_t *)node->child[1],
	    (anna_node_call_t *)node->child[2]);
}
/*
static anna_node_t *anna_macro_else(anna_node_call_t *node,
				    anna_function_t *function, 
				    anna_node_list_t *parent)
{

    CHECK_CHILD_COUNT(node,L"else macro", 1);
    CHECK_NODE_BLOCK(node->child[0]);
    CHECK(parent->idx > 0,node, L"else with no matching if call");
    
    anna_node_call_t *parent_call = node_cast_call(parent->node);
    anna_node_t *prev = parent_call->child[parent->idx-1];

    anna_node_call_t *prev_call = node_cast_call(prev);
    anna_node_identifier_t *prev_call_name = node_cast_identifier(prev_call->function);
    
    CHECK(wcscmp(prev_call_name->name, L"__if__")==0,node, L"else with no matching if call");
    CHECK(prev_call->child_count == 3,prev_call, L"Bad if call");
    CHECK(prev_call->child[2]->node_type == ANNA_NODE_NULL,prev_call, L"Previous if statement already has an else clause");
    
    prev_call->child[2] = anna_node_macro_expand(node->child[0]);
    
    return (anna_node_t *)
	anna_node_create_null(&node->location);
   
}

static anna_object_t *anna_function_or(anna_object_t **param)
{
    return param[0] == null_object?anna_function_wrapped_invoke(param[1], 0, 0, 0):param[0];
}

static anna_node_t *anna_macro_or(anna_node_call_t *node, anna_function_t *function, anna_node_list_t *parent)
{
    CHECK_CHILD_COUNT(node,L"or operator", 2);
    
    anna_node_t *cond1 = anna_node_clone_deep(node->child[0]);
    anna_node_t *cond2 = anna_node_clone_deep(node->child[1]);

    anna_node_macro_expand_children(node);

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
	    cond1,
	    (anna_node_t *)
	    anna_node_create_dummy(
		&node->location,
		anna_function_wrap(
		    anna_function_create(
			anna_util_identifier_generate(
			    L"!orConditionBlock", 
			    &(node->location)), 0, 
		    anna_node_create_call(
			&node->location,
			(anna_node_t *)
			anna_node_create_identifier(
			    &node->location,
			    L"__block__"),
			1,
			&cond2),
		    t2, 0, 0, 0, 
		    function->stack_template,
		    function->return_pop_count+1)),
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
	anna_node_create_call(
	    &node->location, 
	    (anna_node_t *)
	    anna_node_create_dummy( 
		&node->location,
		anna_function_wrap(
		    anna_native_create(
			anna_util_identifier_generate(
			    L"orAnonymous",
			    &(node->location)),
			0,
			(anna_native_t)anna_function_or,
			return_type,
			2,
			argv,
			argn,
			0)),
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
    
    anna_node_macro_expand_child(node, 1);
    
    anna_type_t *t2 = anna_node_get_return_type(node->child[1], function->stack_template);    
    
    CHECK(t2,node->child[1], L"Unknown type for second argument to while");	

    anna_node_t *condition = 
	(anna_node_t *)
	anna_node_create_dummy(
	    &node->location,
	    anna_function_wrap(
		anna_function_create(
		    anna_util_identifier_generate(
			L"whileConditionBlock",
			&(node->location)), 0, 
		    anna_node_create_call(
			&node->location,
			(anna_node_t *)anna_node_create_identifier(
			    &node->location,
			    L"__block__"),
			1,
			&node->child[0]), 
		    t2, 0, 0, 0, 
		    function->stack_template,
		    function->return_pop_count+1)),
	    1);
    
    condition = anna_node_macro_expand(condition);

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

//      FIXME: I think the return values are all wrong here, need to
//      make sure we're rturning the function result, not the function
//      type itself...

    return (anna_node_t *)
	anna_node_create_call(
	    &node->location,
	    (anna_node_t *)anna_node_create_dummy( 
		&node->location,
		anna_function_wrap(
		    anna_native_create(
			anna_util_identifier_generate(
			    L"whileAnonymous",
			    &(node->location)),
			0,
			(anna_native_t)anna_function_while,
			t2,
			2,
			argv,
			argn,
			0)),
		0),
	    2,
	    param);
}
*/
