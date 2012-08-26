void anna_alloc_mark_permanent(void *alloc)
{
    al_push(&anna_alloc_permanent, alloc);
}

static void anna_alloc_unmark(void *obj)
{
    *((int *)obj) &= (~ANNA_USED);
}

void anna_alloc_mark_function(anna_function_t *o)
{
    if( o->flags & ANNA_USED)
	return;
    o->flags |= ANNA_USED;

    if(!o->wrapper)
	return;
        
    anna_function_type_t *ft = anna_function_type_unwrap(o->wrapper->type);
    int i;
    for(i=0; i<o->input_count; i++)
    {
	if(o->input_default[i])
	{
	    anna_alloc_mark_node(o->input_default[i]);
	    anna_alloc_mark_node(ft->input_default[i]);
	}
    }
    
    anna_alloc_mark_node((anna_node_t *)o->attribute);
//    anna_message(L"WEE %ls\n", o->return_type->name);
    
    anna_alloc_mark_type(o->return_type);
    anna_alloc_mark_object(o->wrapper);
    if(o->this)
	anna_alloc_mark_object(o->this);
    if(o->stack_template)
	anna_alloc_mark_stack_template(o->stack_template);

    for(i=0; i<o->input_count; i++)
    {
#ifdef ANNA_CHECK_GC
	if(!o->input_type[i])
	{
	    anna_message(L"No type specified for input argument %d of function %ls\n", 
		    i+1, o->name);
	    CRASH;
	}
#endif	
	anna_alloc_mark_type(o->input_type[i]);
    }

    if(o->code)
	anna_vm_mark_code(o);
}

void anna_alloc_mark_stack_template(anna_stack_template_t *stack)
{
    if( stack->flags & ANNA_USED)
	return;
    stack->flags |= ANNA_USED;

    if(stack->parent)
	anna_alloc_mark_stack_template(stack->parent);
    if(stack->function)
	anna_alloc_mark_function(stack->function);
    if(stack->wrapper)
	anna_alloc_mark_object(stack->wrapper);
    int i;
    for(i=0; i<stack->count; i++)
    {
	if(stack->member_declare_node[i])
	{
	    anna_alloc_mark_node((anna_node_t *)stack->member_declare_node[i]);
	}
    }
    for(i=0; i<al_get_count(&stack->expand); i++)
    {
	anna_use_t *use = al_get(&stack->expand, i);
	anna_alloc_mark_node(use->node);
	anna_alloc_mark_type(use->type);
    }
    for(i=0; i<al_get_count(&stack->import); i++)
    {
	anna_use_t *use = al_get(&stack->import, i);
	anna_alloc_mark_node(use->node);
	anna_alloc_mark_type(use->type);
    }

}

void anna_alloc_mark_node(anna_node_t *o)
{
    if( o->flags & ANNA_USED)
	return;
    o->flags |= ANNA_USED;

    anna_node_t *this = o;

    if(o->wrapper)
	anna_alloc_mark_object(o->wrapper);
    
    switch(this->node_type)
    {
	
	case ANNA_NODE_NULL:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_STRING_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_IDENTIFIER:
	case ANNA_NODE_INTERNAL_IDENTIFIER:
	{
	    break;
	}
	
	case ANNA_NODE_RETURN:
	case ANNA_NODE_BREAK:
	case ANNA_NODE_CONTINUE:
	case ANNA_NODE_TYPE_OF:
	case ANNA_NODE_INPUT_TYPE_OF:
	case ANNA_NODE_RETURN_TYPE_OF:
	case ANNA_NODE_USE:
	{
	    anna_node_wrapper_t *n = (anna_node_wrapper_t *)this;
	    anna_alloc_mark_node(n->payload);
	    break;
	}	
	
	case ANNA_NODE_CAST:
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_CALL:
	case ANNA_NODE_SPECIALIZE:
	case ANNA_NODE_NOTHING:
	{	    
	    anna_node_call_t *n = (anna_node_call_t *)this;
	    int i;
#ifdef ANNA_CHECK_GC
	    if(!n->function)
	    {
		anna_error(n, L"Critical: Invalid AST node");
		anna_node_print(5, n);		
		CRASH;
	    }
#endif
	    if(n->function)
		anna_alloc_mark_node(n->function);
	    for(i=0; i<n->child_count; i++)
	    {
		anna_alloc_mark_node(n->child[i]);
	    }
	    
	    break;
	}
	
	
	case ANNA_NODE_MEMBER_CALL:
	case ANNA_NODE_STATIC_MEMBER_CALL:
	{	    
	    anna_node_call_t *n = (anna_node_call_t *)this;
	    int i;
		
	    anna_alloc_mark_node(n->object);
	    for(i=0; i<n->child_count; i++)
	    {
		anna_alloc_mark_node(n->child[i]);
	    }
	    
	    break;
	}
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)this;	    
	    anna_alloc_mark_function(c->payload);
	    break;
	}
	
	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *c = (anna_node_type_t *)this;
	    anna_type_t *f = c->payload;
	    anna_alloc_mark_type(f);
	    break;
	}

	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *c = (anna_node_assign_t *)this;
	    anna_alloc_mark_node(c->value);
	    break;
	}


	case ANNA_NODE_MEMBER_BIND:
	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_STATIC_MEMBER_GET:
	{
	    anna_node_member_access_t *c = (anna_node_member_access_t *)this;
	    anna_alloc_mark_node(c->object);
	    break;
	}

	case ANNA_NODE_MEMBER_SET:
	case ANNA_NODE_STATIC_MEMBER_SET:
	{
	    anna_node_member_access_t *g = (anna_node_member_access_t *)this;
	    anna_alloc_mark_node(g->object);
	    anna_alloc_mark_node(g->value);
	    break;
	}
	
	case ANNA_NODE_DECLARE:
	case ANNA_NODE_CONST:
	{
	    anna_node_declare_t *d = (anna_node_declare_t *)this;
	    anna_alloc_mark_node(d->value);
	    anna_alloc_mark_node(d->type);
	    break;
	}
	
	case ANNA_NODE_MAPPING:
	case ANNA_NODE_OR:
	case ANNA_NODE_AND:
	case ANNA_NODE_WHILE:
	{
	    anna_node_cond_t *d = (anna_node_cond_t *)this;
	    anna_alloc_mark_node(d->arg1);
	    anna_alloc_mark_node(d->arg2);
	    break;   
	}

	case ANNA_NODE_IF:
	{
	    anna_node_if_t *d = (anna_node_if_t *)this;
	    anna_alloc_mark_node(d->cond);
	    anna_alloc_mark_node((anna_node_t *)d->block1);
	    anna_alloc_mark_node((anna_node_t *)d->block2);
	    break;
	}	
	
	case ANNA_NODE_DUMMY:
	{
	    anna_node_dummy_t *d = (anna_node_dummy_t *)this;
	    anna_alloc_mark_object(d->payload);
	    break;   
	}

	default:
	{
	    anna_error(
		this,
		L"Don't know how to handle node of type %d during garbage collection", this->node_type);	    
	    CRASH;
	    break;
	}
    }
}


static void anna_alloc_mark_blob(void *mem)
{
    int *mem2 = (int *)mem;
    *mem2 |= ANNA_USED;
}

void anna_alloc_mark_entry(anna_entry_t e)
{
    if(anna_entry_null_ptr(e))
	return;
    
    if(!anna_is_obj(e))
    {
#ifndef NAN_BOXING
	if(anna_is_float(e))
	{
	    anna_alloc_mark_blob(anna_as_float_payload(e));
	    return;
	}
#endif
	return;
    }
    anna_object_t *obj = anna_as_obj_fast(e);
    anna_alloc_mark_object(obj);
}

static void anna_alloc_mark_activation_frame(anna_activation_frame_t *frame)
{
    if( frame->flags & ANNA_USED)
	return;
    frame->flags |= ANNA_USED;    

    int i;
    
    for(i=0; i<frame->function->variable_count; i++)
    {	
	anna_alloc_mark_entry(frame->slot[i]);
    }
    if(frame->dynamic_frame)
	anna_alloc_mark_activation_frame(frame->dynamic_frame);
    anna_alloc_mark_function(frame->function);
}

static void anna_alloc_mark_context(anna_context_t *context)
{
    anna_entry_t *obj;
    for(obj = &context->stack[0]; obj < context->top; obj++)
    {	
	anna_alloc_mark_entry(*obj);
    }
    anna_alloc_mark_activation_frame(context->frame);
}

void anna_alloc_mark(void *obj)
{
    switch(*((int *)obj) & ANNA_ALLOC_MASK)
    {
	case ANNA_OBJECT:
	{
	    anna_alloc_mark_object((anna_object_t *)obj);
	    break;
	}
	case ANNA_TYPE:
	{
	    anna_alloc_mark_type((anna_type_t *)obj);
	    break;
	}
	case ANNA_FUNCTION:
	{
	    anna_alloc_mark_function((anna_function_t *)obj);
	    break;
	}
	case ANNA_NODE:
	{
	    anna_alloc_mark_node((anna_node_t *)obj);
	    break;
	}
	case ANNA_STACK_TEMPLATE:
	{
	    anna_alloc_mark_stack_template((anna_stack_template_t *)obj);
	    break;
	}
	case ANNA_ACTIVATION_FRAME:
	{
	    anna_alloc_mark_activation_frame((anna_activation_frame_t *)obj);
	    break;
	}
	case ANNA_FUNCTION_TYPE:
	{
	    break;
	}
	case ANNA_BLOB:
	{
	    anna_alloc_mark_blob(obj);
	    break;
	}
	default:
	{
	    anna_error(0, L"Tried to mark unknown memory region in GC. Type: %d\n", (*((int *)obj) & ANNA_ALLOC_MASK));	    
	    CRASH;
	}
    }
}
