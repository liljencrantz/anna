#if 0

static anna_node_t *anna_macro_operator_wrapper(anna_node_call_t *node, 
						anna_function_t *function, 
						anna_node_list_t *parent)
{
    return 0;
/*
    wprintf(L"\noperator wrapper called with %d children\n", node->child_count);
   
    anna_node_print(node);
    wprintf(L"\n");
*/ 
    CHECK(node->child_count >=2,node, L"Too few arguments");
    CHECK(node->child_count <=3,node, L"Too many arguments");
    anna_node_macro_expand_children(node);
    int arg_offset = 0;
    anna_type_t * t1;
    anna_type_t * t2;
    wchar_t *name_prefix;
    if(node->child_count == 2)
    {
       anna_node_identifier_t *name_identifier = node_cast_identifier(node->function);
       if(wcslen(name_identifier->name) < 5)
       {
	  FAIL(node, L"Invalid operator name: %ls", name_identifier->name);	
       }
       name_prefix = wcsdup(name_identifier->name);
       //wprintf(L"Calling operator_wrapper as %ls\n", name);       
    }
    else 
    {
       anna_node_identifier_t *name_identifier = node_cast_identifier(node->child[0]);
       string_buffer_t sb;
       sb_init(&sb);
       sb_append(&sb, L"__");
       sb_append(&sb, name_identifier->name);
       sb_append(&sb, L"__");
       name_prefix = sb_content(&sb);
       arg_offset = 1;
    }
/*
    wprintf(L"LALALA\n");    
    anna_node_print(node->child[0]);
    wprintf(L"\n");
*/  
    t1 = anna_node_get_return_type(node->child[arg_offset], function->stack_template);
    t2 = anna_node_get_return_type(node->child[arg_offset+1], function->stack_template);
    CHECK(t1,node->child[arg_offset], L"Unknown type for first argument to operator %ls__", name_prefix);
    CHECK(t2,node->child[arg_offset+1], L"Unknown type for second argument to operator %ls__", name_prefix);	
    wchar_t *method_name = anna_find_method((anna_node_t *)node, t1, name_prefix, 2, t2);
    
    if(method_name)
    {
	    
	anna_node_t *mg_param[2]=
	    {
		node->child[arg_offset], (anna_node_t *)anna_node_create_identifier(&node->location,method_name)
	    }
	;
	
	anna_node_t *c_param[1]=
	    {
		node->child[arg_offset+1]
	    }
	;
	
	return (anna_node_t *)
	    anna_node_create_call(
		&node->location,
		(anna_node_t *)anna_node_create_call(
		    &node->location,
		    (anna_node_t *)
		    anna_node_create_identifier(
			&node->location,
			L"__memberGet__"),
		    2,
		    mg_param),
		1,
		c_param);
    }
    else
    {
	string_buffer_t buff;
	sb_init(&buff);
	sb_append(&buff, L"__r");
	sb_append(&buff, &name_prefix[2]);
	wchar_t *reverse_name_prefix = sb_content(&buff);
	method_name = anna_find_method((anna_node_t *)node, t2, reverse_name_prefix, 2, t1);
	sb_destroy(&buff);
	
	if(!method_name)
	{
	    FAIL(node, L"%ls: No support for call with objects of types %ls and %ls\n",
		 name_prefix, t1->name, t2->name);
	}

	anna_node_t *mg_param[2]=
	    {
		node->child[arg_offset+1], (anna_node_t *)anna_node_create_identifier(&node->location, method_name)
	    }
	;
	
	anna_node_t *c_param[1]=
	    {
		node->child[arg_offset]
	    }
	;
	
	return (anna_node_t *)
	    anna_node_create_call(
		&node->location,
		(anna_node_t *)
		anna_node_create_call(
		    &node->location,
		    (anna_node_t *)
		    anna_node_create_identifier(
			&node->location,
			L"__memberGet__"),
		    2,
		    mg_param),
		1,
		c_param);
    }
}
#endif

static anna_node_t *anna_macro_assign(struct anna_node_call *node)
{
    CHECK_CHILD_COUNT(node,L"assignment operator", 2);

    switch(node->child[0]->node_type)
    {

	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *name_identifier = 
		node_cast_identifier(node->child[0]);
	    return (anna_node_t *)
		anna_node_create_assign(&node->location,
					name_identifier->name,
					node->child[1]);
	}
       
	case ANNA_NODE_CALL:
	{

	    anna_node_call_t *call = node_cast_call(node->child[0]);
	    anna_node_print(call->function);

	    if(call->function->node_type == ANNA_NODE_IDENTIFIER)
	    {
		
/*
__assign__(__memberGet__( OBJ, KEY), VL  )
 => 
__memberSet__( OBJ, KEY, VAL)

 */
		anna_node_identifier_t *name_identifier = node_cast_identifier(call->function);
		if(wcscmp(name_identifier->name, L"__memberGet__")==0)
		{
		    // foo.bar = baz
		    call->function = (anna_node_t *)
			anna_node_create_identifier(
			&name_identifier->location, 
			L"__memberSet__");
		    anna_node_call_add_child(
			call, 
			node->child[1]);
		    return (anna_node_t *)call;
		}	    
	    }
	    else if(call->function->node_type == ANNA_NODE_CALL)
	    {
		anna_node_call_t *call2 = node_cast_call(call->function);
		
		if(call2->function->node_type == ANNA_NODE_IDENTIFIER && call2->child_count == 2)
		{
		    
		    anna_node_identifier_t *name_identifier = node_cast_identifier(call2->function);
		    
		    if(wcscmp(name_identifier->name, L"__memberGet__")==0 && call2->child[1]->node_type == ANNA_NODE_IDENTIFIER)
		    {
			anna_node_identifier_t *name_identifier2 = node_cast_identifier(call2->child[1]);
			if(wcscmp(name_identifier2->name, L"__get__")==0)
			{
			    /*
			      __assign__(__memberGet__( OBJ, "__get__")(KEY), val  )
			      
			      __memberGet__( OBJ, "__set__")( KEY, VAL)
			      
			    */
			    call2->child[1] = (anna_node_t *)
				anna_node_create_identifier(
				    &name_identifier->location,
				    L"__set__");
			    anna_node_call_add_child(
				call, 
				node->child[1]);
			    return (anna_node_t *)call;
			}
		    }
		}		
	    }
	}
    }
    FAIL(node->child[0], L"Invalid left-hand value in assignment");
}

static anna_node_t *anna_macro_member_get(anna_node_call_t *node)
{
/*
  wprintf(L"member_get on node at %d\n", node);
  anna_node_print((anna_node_t *)node);
  wprintf(L"\n");
*/
    CHECK_CHILD_COUNT(node,L". operator", 2);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
        
    anna_node_identifier_t *name_node = node_cast_identifier(node->child[1]);
    mid_t mid = anna_mid_get(name_node->name);

    return (anna_node_t *)anna_node_create_member_get(&node->location,
						      node->child[0], 
						      mid);
}

static anna_node_t *anna_macro_member_set(anna_node_call_t *node)
{
/*
  wprintf(L"member_get on node at %d\n", node);
  anna_node_print((anna_node_t *)node);
  wprintf(L"\n");
*/
    CHECK_CHILD_COUNT(node,L"member assignment", 3);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
    
    anna_node_identifier_t *name_node = node_cast_identifier(node->child[1]);
    mid_t mid = anna_mid_get(name_node->name);

    return (anna_node_t *)anna_node_create_member_set(&node->location,
						      node->child[0], 
						      mid,
						      node->child[2]);
}

