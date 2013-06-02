//ROOT: src/node/node.c

/**
   Search the current stack for a macro matching the specified call
 */
static anna_function_t *anna_node_macro_get(anna_node_t *node, anna_stack_template_t *stack)
{
/*
    anna_message(L"Checking for macros in node (%d)\n", node->function->node_type);
    anna_node_print(0, node);
*/
    switch(node->node_type)
    {
	case ANNA_NODE_IDENTIFIER:
	{
//	    anna_message(L"It's an identifier\n");
	    anna_node_identifier_t *name=(anna_node_identifier_t *)node;

	    anna_object_t *obj = anna_as_obj(anna_stack_macro_get(stack, name->name));
	    if(obj && obj != null_object)
	    {
		
		anna_function_t *func=anna_function_unwrap(obj);
		//anna_message(L"Tried to find object %ls on stack, got %d, revealing internal function ptr %d\n", name->name, obj, func);
		
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

/**
   Perform macro expansion on all nodes
*/
static anna_node_t *anna_node_macro_expand_each(
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
	{
	    anna_node_call_t *this2 =(anna_node_call_t *)this;
	    
	    if(!(this2->flags & ANNA_NODE_DONT_EXPAND))
	    {
		anna_function_t *macro = anna_node_macro_get(this2->function, stack);
		if(macro)
		{
		    return anna_node_macro_expand(anna_macro_invoke(macro, this2), stack);
		}
	    }
	    
	    break;
	}
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)this;
	    anna_function_t *f = c->payload;
	    
	    anna_function_specialize_body(f);
	    anna_function_macro_expand(f, stack);
	    
	    break;
	}
	
	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *c = (anna_node_type_t *)this;
	    anna_type_t *f = c->payload;
	    anna_type_macro_expand(f, stack);
	    break;
	}

	case ANNA_NODE_DECLARE:
	case ANNA_NODE_CONST:
	{
	    int i;
	    anna_node_declare_t *c = (anna_node_declare_t *)this;
	    for(i=0;i<c->attribute->child_count; i++)
	    {
//		anna_node_print(999, f->attribute->child[i]);
		
		c->attribute->child[i] = anna_node_macro_expand(
		    c->attribute->child[i], stack);
	    }
	}
	

    }
    return this;
}

/**
   Transform all AST nodes matching the pattern

   __memberGet__(X, identifier)(...)

   into

   __memberCall__(X, identifier, ...)
 */
static anna_node_t *anna_node_macro_member_call(
    anna_node_t *this,
    void *aux)
{
    switch( this->node_type )
    {
	case ANNA_NODE_CALL:
	case ANNA_NODE_CAST:
	{
	    anna_node_call_t *this2 =(anna_node_call_t *)this;
	    if(this->node_type != ANNA_NODE_SPECIALIZE)
	    {
		if((this2->function->node_type == ANNA_NODE_MEMBER_GET) ||
		   (this2->function->node_type == ANNA_NODE_STATIC_MEMBER_GET))
		{
		    anna_node_member_access_t *mg = 
			(anna_node_member_access_t *)this2->function;
		    int type = 
			(this2->function->node_type == ANNA_NODE_STATIC_MEMBER_GET)?
			ANNA_NODE_STATIC_MEMBER_CALL:
			ANNA_NODE_MEMBER_CALL;

		    anna_node_t *result = 
			(anna_node_t *)anna_node_create_member_call(
			    &this2->location,
			    type,
			    mg->object,
			    mg->mid,
			    this2->child_count,
			    this2->child);
		    return result;
		}
	    }
	    break;
	}
    }
    return this;
}    

anna_node_t *anna_node_macro_expand(
    anna_node_t *this,
    anna_stack_template_t *stack)
{
    return anna_node_merge(
	anna_node_each_replace(
	    anna_node_each_replace(
		this, 
		(anna_node_replace_function_t)anna_node_macro_expand_each, stack),
	    anna_node_macro_member_call, 0));
}
