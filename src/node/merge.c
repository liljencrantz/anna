static int is_call(anna_node_t *node)
{
    switch(node->node_type)
    {
	case ANNA_NODE_CALL:
	case ANNA_NODE_MEMBER_CALL:
	case ANNA_NODE_STATIC_MEMBER_CALL:
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_SPECIALIZE:
	{
	    return 1;
	}
    }
    return 0;
}

static int merge_node(anna_node_t *target, anna_node_t *el)
{
		
    if(is_call(target))
    {
	anna_node_call_t *prev = (anna_node_call_t *)target;
	anna_node_call_add_child(prev, el);
	return 1;
    }

    switch(target->node_type)
    {
	case ANNA_NODE_IF:
	{
	    anna_node_if_t *target2 = (anna_node_if_t *)target;
	    if(target2->has_else)
	    {
		anna_error(el, L"Multiple else clauses");
		break;
	    }
	    if(!anna_node_is_call_to(el, L"else"))
	    {
		anna_error(el, L"Invalid else clause");
		break;			
	    }
	    anna_node_call_t *el_call = (anna_node_call_t *)el;
	    if(el_call->child_count != 1 /*|| el_call->child[0]->node_type != ANNA_NODE_CALL*/)
	    {
		anna_error(el, L"Invalid else clause %d", el_call->child[0]->node_type);
		break;
	    }
	    
	    target2->block2 = (anna_node_call_t *)el_call->child[0];
	    target2->has_else = 1;
	    return 1;
	}

	case ANNA_NODE_BREAK:
	case ANNA_NODE_CONTINUE:
	case ANNA_NODE_RETURN:
	{
	    anna_node_wrapper_t *n = (anna_node_wrapper_t *)target;
	    return merge_node(n->payload, el);
	}
	
	case ANNA_NODE_DECLARE:
	case ANNA_NODE_CONST:
	{
	    anna_node_declare_t *n = (anna_node_declare_t *)target;
	    return merge_node(n->value, el);
	}
	
	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *n = (anna_node_assign_t *)target;
	    return merge_node(n->value, el);
	}
	
	default:
	{
	    anna_error(el, L"Can't merge node - invalid context");
	    break;
	}
    }
    return 0;
}

static void anna_node_merge_each(anna_node_t *node, void *aux)
{
    if(is_call(node))
    {
/*	wprintf(L"merge each\n");
	anna_node_print(5, node);
	wprintf(L"\n");
*/	
	anna_node_call_t *call = (anna_node_call_t *)node;
	int i;
	for(i=0; i<call->child_count; i++)
	{
	    anna_node_t *el = call->child[i];
	    if(el->flags & ANNA_NODE_MERGE)
	    {
		//anna_node_print(5, node);
		if(i == 0)
		{
		    anna_error(el, L"Failed to merge node");
		    continue;		    
		}
		if(merge_node(call->child[i-1], el))
		{
		    
		    memmove(
			&call->child[i], &call->child[i+1],
			sizeof(anna_entry_t *)*(call->child_count -i-1));
		    i--;
		    call->child_count--;
		}	
	    }
	}
    }
/*    else
    {
	wprintf(L"skip each\n");
	anna_node_print(5, node);
	wprintf(L"\n");
    }
*/  
}

anna_node_t *anna_node_merge(
    anna_node_t *this)
{
    anna_node_each(
	this, 
	anna_node_merge_each, 0);
    return this;
}
