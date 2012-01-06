
ANNA_VM_MACRO(anna_macro_assign)
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
	    //    anna_node_print(0, call->function);

	    if(call->function->node_type == ANNA_NODE_IDENTIFIER)
	    {
		
/*
__assign__(__memberGet__( OBJ, KEY), VL  )
 => 
__memberSet__( OBJ, KEY, VAL)

 */
		anna_node_identifier_t *name_identifier = node_cast_identifier(call->function);
		int is_set = (wcscmp(name_identifier->name, L"__memberGet__")==0) || (wcscmp(name_identifier->name, L"__staticMemberGet__")==0);
		int is_static = wcscmp(name_identifier->name, L"__staticMemberGet__")==0;
		
		if(is_set)
		{
		    // foo.bar = baz
		    call->function = (anna_node_t *)
			anna_node_create_identifier(
			&name_identifier->location, 
			is_static ? L"__staticMemberSet__":L"__memberSet__");
		    anna_node_call_add_child(
			call, 
			node->child[1]);
		    return (anna_node_t *)call;
		}	    
		else if(anna_node_is_call_to(node->child[0], L"__collection__"))
		{
		    node->function = anna_node_create_identifier(
			&node->child[0]->location, 
			L"__assignList__");
		    return (anna_node_t *)node;
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

ANNA_VM_MACRO(anna_macro_member_get)
{
/*
  wprintf(L"member_get on node at %d\n", node);
  anna_node_print(0, (anna_node_t *)node);
  wprintf(L"\n");
*/
    CHECK_CHILD_COUNT(node,L". operator", 2);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
        
    anna_node_identifier_t *name_node = node_cast_identifier(node->child[1]);
    mid_t mid = anna_mid_get(name_node->name);

    return (anna_node_t *)anna_node_create_member_get(
	&node->location,
	ANNA_NODE_MEMBER_GET,
	node->child[0], 
	mid);
}

ANNA_VM_MACRO(anna_macro_static_member_get)
{
/*
  wprintf(L"member_get on node at %d\n", node);
  anna_node_print(0, (anna_node_t *)node);
  wprintf(L"\n");
*/
    CHECK_CHILD_COUNT(node,L". operator", 2);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
        
    anna_node_identifier_t *name_node = node_cast_identifier(node->child[1]);
    mid_t mid = anna_mid_get(name_node->name);

    return (anna_node_t *)anna_node_create_member_get(
	&node->location,
	ANNA_NODE_STATIC_MEMBER_GET,
	node->child[0], 
	mid);
}

 
ANNA_VM_MACRO(anna_macro_member_set)
{
    CHECK_CHILD_COUNT(node,L"member assignment", 3);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
    
    anna_node_identifier_t *name_node = node_cast_identifier(node->child[1]);
    mid_t mid = anna_mid_get(name_node->name);

    return (anna_node_t *)anna_node_create_member_set(
	&node->location,
	ANNA_NODE_MEMBER_SET,
	node->child[0], 
	mid,
	node->child[2]);
}

ANNA_VM_MACRO(anna_macro_static_member_set)
{
    CHECK_CHILD_COUNT(node,L"static member assignment", 3);
    CHECK_NODE_TYPE(node->child[1], ANNA_NODE_IDENTIFIER);
    
    anna_node_identifier_t *name_node = node_cast_identifier(node->child[1]);
    mid_t mid = anna_mid_get(name_node->name);

    return (anna_node_t *)anna_node_create_member_set(
	&node->location,
	ANNA_NODE_STATIC_MEMBER_SET,
	node->child[0], 
	mid,
	node->child[2]);
}
