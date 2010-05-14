

static anna_node_t *anna_prepare_function_common(anna_function_t *function)
{
    int is_variadic=0;
    if(function->flags & ANNA_FUNCTION_PREPARED_COMMON)
	return 0;
    
    anna_type_t *type = function->member_of;
    
    anna_node_call_t *node = function->definition;
    if(!node){
	return 0;
    }

    int verbose = (wcscmp(function->name, L"mandelbrottt") == 0);
    if(verbose)
	wprintf(L"Start common init of function of %ls %d\n", function->name, verbose);	
    
    anna_node_t *body = node->child[4];
    if(body->node_type != ANNA_NODE_NULL && 
       body->node_type != ANNA_NODE_CALL &&
       body->node_type != ANNA_NODE_DUMMY)
    {
        FAIL(node, L"Invalid function body");
    }
    
    if(body->node_type == ANNA_NODE_CALL) {
	function->body = (anna_node_call_t *)body;
    }
    else if(body->node_type == ANNA_NODE_DUMMY)
    {
	anna_node_dummy_t *body_dummy = (anna_node_dummy_t *)body;
	function->native = (anna_native_t)(anna_native_function_t)body_dummy->payload;	
    }
    else
    {
	function->native = (anna_native_t)anna_i_null_function;	
    }

    anna_node_list_t list = 
	{
	    (anna_node_t *)function->body, 0, 0
	}
    ;

    size_t argc=0;
    anna_type_t **argv=0;
    wchar_t **argn=0;
    
    CHECK_NODE_TYPE(node->child[2], ANNA_NODE_CALL);
    anna_node_call_t *declarations = node_cast_call(node->child[2]);
    int i;
    if(declarations->child_count > 0 || type)
    {
	argc = declarations->child_count;
	if(type)
	{
	    argc++;
	}
	
	if(verbose)
	    wprintf(
		L"Adding input arguments to function\n");
	
		
	argv = malloc(sizeof(anna_type_t *)*argc);
	argn = malloc(sizeof(wchar_t *)*argc);
	
	if(type)
	{
	    argv[0]=type;
	    argn[0]=L"this";
//	    wprintf(L"Function %ls is method, add 'this' variable\n", function->name);
	    
	}
	else
	{
	    
//	    wprintf(L"Function %ls is not a method, do not add 'this' variable\n", function->name);
	}
	
	for(i=0; i<declarations->child_count; i++)
	{
	    //declarations->child[i] = anna_node_prepare(declarations->child[i], function, parent);
	    CHECK_NODE_TYPE(declarations->child[i], ANNA_NODE_CALL);
	    anna_node_call_t *decl = node_cast_call(declarations->child[i]);
	    
	    CHECK_NODE_TYPE(decl->function, ANNA_NODE_IDENTIFIER);
	    anna_node_identifier_t *fun = node_cast_identifier(decl->function);
	    if(wcscmp(fun->name, L"__declare__") == 0)
	    {
		//CHECK_CHILD_COUNT(decl, L"variable declaration", 3);
		CHECK_NODE_TYPE(decl->child[0], ANNA_NODE_IDENTIFIER);
		anna_node_prepare_child(
		    decl,
		    1,
		    function,
		    &list);
		anna_node_identifier_t *name = 
		    node_cast_identifier(
			decl->child[0]);
		
		anna_node_identifier_t *type_name =
		    node_cast_identifier(decl->child[1]);
		
		anna_object_t **type_wrapper =
		    anna_stack_addr_get_str(function->stack_template, type_name->name);
		CHECK(
		    type_wrapper, 
		    (anna_node_t *)type_name,
		    L"Unknown type: %ls",
		    type_name->name);

		argv[i+!!type] = 
		    anna_type_unwrap(*type_wrapper);
		argn[i+!!type] = name->name;		
		
		if(verbose)
		    wprintf(
			L"Adding %ls\n", name->name);
	

		if(decl->child_count ==3 &&
		   decl->child[2]->node_type == ANNA_NODE_IDENTIFIER) 
		{
		    anna_node_identifier_t *def =
			node_cast_identifier(decl->child[2]);
		    if(wcscmp(def->name,L"__variadic__") == 0)
		    {
			is_variadic = 1;
			if(i != (declarations->child_count-1))
			{
			    FAIL(def, L"Only the last argument to a function can be variadic");
			}
/*			else
			{
			    wprintf(L"Variadic function\n");
			}
*/
		    }
		}
	    }
	    else if(wcscmp(fun->name, L"__function__") == 0)
	    {
		anna_node_t *fun_node = 
		    anna_node_prepare((anna_node_t *)decl, function, &list);
		//anna_node_print(fun_node);
		
		CHECK_NODE_TYPE(fun_node, ANNA_NODE_TRAMPOLINE);
		anna_node_dummy_t *fun_dummy = (anna_node_dummy_t *)fun_node;
		anna_function_t *fun = anna_function_unwrap(
		    fun_dummy->payload);
                CHECK(fun, decl, L"Could not parse function declaration");              
		anna_prepare_function_interface(fun);
		
                argv[i+!!type] = anna_function_wrap(fun)->type;
                argn[i+!!type] = fun->name;
		
/*
                anna_node_t *fun_wrap = anna_prepare_function_interface(
                    0, decl, function, 
                    parent, 0);

                CHECK_NODE_TYPE(fun_wrap, ANNA_NODE_DUMMY);
                anna_node_dummy_t *fun_dummy = (anna_node_dummy_t *)fun_wrap;
                anna_function_t *fun = anna_function_unwrap(fun_dummy->payload);
                CHECK(fun, decl, L"Could not parse function declaration");              
                argv[i+!!type] = anna_function_wrap(fun)->type;
                argn[i+!!type] = fun->name;

*/
	    }
	    else 
	    {
		FAIL(decl, L"Unknown declaration type");
	    }
	    
	}
    }

    function->flags |= (is_variadic?ANNA_FUNCTION_VARIADIC:0);
    function->input_count = argc;
    function->input_name = argn;
    function->input_type = argv;

    if(!(function->flags & ANNA_FUNCTION_MACRO)) 
    {
	int is_variadic = ANNA_IS_VARIADIC(function);
	for(i=0; i<function->input_count-is_variadic;i++)
	{
	    anna_stack_declare(
		function->stack_template,
		function->input_name[i], 
		function->input_type[i], 
		null_object);	
	}
	if(is_variadic)
	{
	      //FIXME:
	      //Templatize to right list subtype
	    anna_stack_declare(
		function->stack_template, 
		function->input_name[function->input_count-1], 
		list_type, 
		null_object);
	}
    }
    else
    {
	anna_stack_declare(
	    function->stack_template, 
	    function->input_name[0], 
	    node_call_wrapper_type,
	    null_object);
    }

    function->flags |= ANNA_FUNCTION_PREPARED_COMMON;
    
    if(verbose)
	wprintf(L"Finish common init of function of %ls\n", function->name);	

    return 0;
}


static anna_node_t *anna_prepare_function_interface_internal(
    anna_function_t *function)
{
    anna_node_call_t *node = function->definition;
    int is_module = !!(function->flags & ANNA_FUNCTION_MODULE);
    int i;
    
    if(is_module)
    {
//	wprintf(L"Holy crap, «%ls» function is a module\n", function->name);
    }
    

    if(function->flags & ANNA_FUNCTION_PREPARED_INTERFACE)
	return 0;
    
    if(node && node->child[0]->node_type != ANNA_NODE_NULL)
    {
/*
	wprintf(L"\n\nCreate function interface from node:\n");
	anna_node_print((anna_node_t *)node);
*/
    }
    
    CHECK_CHILD_COUNT(node,L"function definition", 5);
    //wprintf(L"Preparing interface of function %ls\n", function->name);
    
    if(anna_prepare_function_common(function))
	return 0;
    
    anna_type_t *out_type=0;
    anna_node_t *out_type_wrapper = node->child[1];
    
    anna_node_list_t list = 
	{
	    (anna_node_t *)function->body, 0, 0
	}
    ;

    if(out_type_wrapper->node_type == ANNA_NODE_NULL) 
    {	
	CHECK(
	    function->body->node_type == ANNA_NODE_CALL, 
	    function->body, 
	    L"Function declarations must have a return type");
	anna_sniff_return_type(function);
	out_type = function->return_type;
	if(!out_type)
	{
	    wprintf(
		L"Critical: Failed to sniff return type of function %ls\n",
		function->name);
	    if(function->definition)
	    {
		anna_node_print((anna_node_t *)function->definition);
	    }
	    CRASH;
	}
    }
    else
    {
        anna_node_prepare_child(node, 1, function, &list);
	out_type_wrapper = node->child[1];
	anna_node_identifier_t *type_identifier;
	type_identifier = node_cast_identifier(out_type_wrapper);
	anna_object_t *type_wrapper = anna_stack_get_str(
	    function->stack_template, 
	    type_identifier->name);

	if(!type_wrapper)
	{
	    FAIL(type_identifier, L"Unknown type: %ls", type_identifier->name);
	}
	out_type = anna_type_unwrap(type_wrapper);
    }
    function->return_type = out_type;
    
    anna_function_setup_type(function, function->stack_template->parent);
    
    if(is_module)
    {
	for(i=0; i<al_get_count(&function->child_function); i++) 
	{
	    anna_function_t *func = (anna_function_t *)al_get(&function->child_function, i);
/*
  wprintf(L"Prepare subfunction %d of %d in function %ls: %ls\n", 
  i, al_get_count(&function->child_function),
  function->name, func->name);
*/	
	    anna_prepare_function_interface(func);
	    
	//anna_function_t *func = (anna_function_t *)al_get(&function->child_function, i);
/*
	wprintf(L"Prepare subfunction %d of %d in function %ls: %ls\n", 
		i, al_get_count(&function->child_function),
		function->name, func->name);
*/	
//	if(func->flags & ANNA_FUNCTION_MACRO)
//	    anna_prepare_function_internal(func, &current);
	}
    
	
    }
    



/*    
    if(!is_anonymous)
    {
	anna_stack_declare(
	    function->stack_template->parent, 
	    name, 
	    anna_type_for_function(
		function->return_type,
		function->input_count,
		function->input_type,
		function->input_name,
		is_variadic),
	    anna_function_wrap(function));
    }
*/



    function->flags |= ANNA_FUNCTION_PREPARED_INTERFACE;
    return (anna_node_t *)anna_node_create_dummy(&node->location, anna_function_wrap(function),0);
}

void anna_prepare_function_interface(
    anna_function_t *function)
{
    prepare_item_t it = 
	{
	    function, 0, L"Interface preparation"
	}
    ;
    if(anna_prepare_check(&it))
	return;
    
    anna_prepare_function_interface_internal(function);
    anna_prepare_pop();
    

}
;


void anna_prepare_function(anna_function_t *function)
{
    prepare_item_t it = 
	{
	    function, 0, L"Body preparation"
	}
    ;
    if(anna_prepare_check(&it))
	return;
    
    anna_prepare_function_internal(function);
    anna_prepare_pop();
    
}

static void anna_prepare_function_internal(
    anna_function_t *function)
{
    int i;
    anna_node_list_t list = 
	{
	    (anna_node_t *)function->body, 0, 0
	}
    ;
    
    //anna_prepare_function_interface(function);
    
    if(anna_function_prepared(function))
	return;
    
    if(anna_prepare_function_common(function))
	return;
    
    //wprintf(L"Preparing body of function %ls\n", function->name);
    

    if(function->body)
    {
	for(i=0; i<function->body->child_count; i++) 
	{
	    list.idx=i;
	    function->body->child[i] = anna_node_prepare(
		function->body->child[i],
		function, 
		&list);
	}
    
	for(i=0; i<function->body->child_count; i++) 
	{
	    anna_node_validate(function->body->child[i], function->stack_template);
	}
    }

    function->flags |= ANNA_FUNCTION_PREPARED_IMPLEMENTATION;
}

static void anna_prepare_function_recursive(anna_function_t *block)
{
    anna_prepare_function(block);
    
    int i;
    
    for(i=0; i<al_get_count(&block->child_function); i++) 
    {
	anna_function_t *func = (anna_function_t *)al_get(&block->child_function, i);
	anna_prepare_function_recursive(func);
    }
}

