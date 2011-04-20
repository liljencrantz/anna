
static anna_function_t *anna_node_macro_get(anna_node_t *node, anna_stack_template_t *stack)
{
/*
    wprintf(L"Checking for macros in node (%d)\n", node->function->node_type);
    anna_node_print(0, node);
*/
    switch(node->node_type)
    {
	case ANNA_NODE_IDENTIFIER:
	{
//	    wprintf(L"It's an identifier\n");
	    anna_node_identifier_t *name=(anna_node_identifier_t *)node;

	    anna_object_t *obj = anna_stack_macro_get(stack, name->name);
	    if(obj && obj != null_object)
	    {
		
		anna_function_t *func=anna_function_unwrap(obj);
		//wprintf(L"Tried to find object %ls on stack, got %d, revealing internal function ptr %d\n", name->name, obj, func);
		
		if(func && (func->flags & ANNA_FUNCTION_MACRO))
		{
		    return func;
		}
	    }
	    
	    break;
	}

	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *call=(anna_node_call_t *)node;
	    if(anna_node_is_named(call->function, L"__memberGet__") && (call->child_count == 2))
	    {
		return anna_node_macro_get(
		    call->child[1], stack);
	    }	
	    break;
	}
	
    }
    return 0;
    
}


anna_node_t *anna_node_macro_expand(
    anna_node_t *this,
    anna_stack_template_t *stack)
{
/*
    debug(D_SPAM,L"EXPAND\n");
    anna_node_print(0, this);
*/  
    switch( this->node_type )
    {
	case ANNA_NODE_CALL:
	case ANNA_NODE_CAST:
	case ANNA_NODE_SPECIALIZE:
	{
	    anna_node_call_t *this2 =(anna_node_call_t *)this;
	    anna_function_t *macro = anna_node_macro_get(this2->function, stack);
	    
	    if(macro)
	    {
		anna_node_t *res = anna_macro_invoke(macro, this2);
		return anna_node_macro_expand(res, stack);
	    }
	    
	    this2->function = anna_node_macro_expand(this2->function, stack);
	    
	    int i;
	    for(i=0;i<this2->child_count;i++)
	    {
		this2->child[i] = anna_node_macro_expand(this2->child[i], stack);
	    }

	    if(this->node_type != ANNA_NODE_SPECIALIZE)
	    {
		if(this2->function->node_type == ANNA_NODE_MEMBER_GET)
		{
		    anna_node_member_access_t *mg = 
			(anna_node_member_access_t *)this2->function;
		    
		    anna_node_t *result = 
			(anna_node_t *)anna_node_create_member_call(
			    &this2->location,
			    mg->object,
			    mg->mid,
			    this2->child_count,
			    this2->child);
		    return result;
		}
	    }

	    return this;
	}
	
	case ANNA_NODE_IDENTIFIER:
	case ANNA_NODE_MAPPING_IDENTIFIER:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_STRING_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_NULL:
	case ANNA_NODE_DUMMY:
	{
	    return this;
	}
	
	case ANNA_NODE_RETURN:
	case ANNA_NODE_TYPE_LOOKUP:
	{
	    anna_node_wrapper_t *c = (anna_node_wrapper_t *)this;
	    c->payload = anna_node_macro_expand(c->payload, stack);
	    break;
	}

	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)this;
	    anna_function_t *f = c->payload;
	    
	    if(f->body)
	    {
		int i;
		for(i=0;i<f->body->child_count; i++)
		    f->body->child[i] = anna_node_macro_expand(
			f->body->child[i], stack);
	    }
	    return this;
	}

	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *c = (anna_node_type_t *)this;
	    anna_type_t *f = c->payload;
	    
	    anna_type_macro_expand(f, stack);
	    
	    return this;
	}

	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_MEMBER_GET_WRAP:
	{
	    anna_node_member_access_t *g = (anna_node_member_access_t *)this;
	    g->object = anna_node_macro_expand(g->object, stack);
	    return this;
	}

	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_access_t *g = (anna_node_member_access_t *)this;
	    g->object = anna_node_macro_expand(g->object, stack);
	    g->value = anna_node_macro_expand(g->value, stack);
	    return this;
	}

	case ANNA_NODE_DECLARE:
	case ANNA_NODE_CONST:
	{
	    anna_node_declare_t *d = (anna_node_declare_t *)this;
	    d->type = anna_node_macro_expand(d->type, stack);
	    d->value = anna_node_macro_expand(d->value, stack);
	    return this;
	}	
	
	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *d = (anna_node_assign_t *)this;
	    d->value = anna_node_macro_expand(d->value, stack);
	    return this;
	}	
	
	case ANNA_NODE_WHILE:
	case ANNA_NODE_AND:
	case ANNA_NODE_OR:
	case ANNA_NODE_MAPPING:
	{
	    anna_node_cond_t *c = (anna_node_cond_t *)this;
	    c->arg1 = anna_node_macro_expand(c->arg1, stack);
	    c->arg2 = anna_node_macro_expand(c->arg2, stack);
	    return this;
	}	
	
	case ANNA_NODE_IF:
	{
	    anna_node_if_t *c = (anna_node_if_t *)this;
	    c->cond = anna_node_macro_expand(c->cond, stack);
	    c->block1 = (anna_node_call_t *)anna_node_macro_expand(
		(anna_node_t *)c->block1, stack);
	    c->block2 = (anna_node_call_t *)anna_node_macro_expand(
		(anna_node_t *)c->block2, stack);
	    return this;
	}	
	
	default:
	{
	    anna_error(
		this,
		L"Invalid node of type %d during macro expansion", this->node_type);
	}
    }

    return this;
}

