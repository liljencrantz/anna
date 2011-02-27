
#define ANNA_NODE_TYPE_IN_TRANSIT ((anna_type_t *)1)


anna_node_t *anna_node_macro_expand(
    anna_node_t *this,
    anna_stack_frame_t *stack)
{
/*
    debug(0,L"EXPAND\n");
    anna_node_print(this);
*/  
    switch( this->node_type )
    {
	case ANNA_NODE_CALL:
	case ANNA_NODE_SPECIALIZE:
	{
	    anna_node_call_t *this2 =(anna_node_call_t *)this;
	    
	    if(this2->function->node_type == ANNA_NODE_CALL)
	    {
		anna_node_call_t *c = (anna_node_call_t *)this2->function;
		if(c->function->node_type == ANNA_NODE_IDENTIFIER && c->child_count==2)
		{
		    anna_node_identifier_t *mgfun = (anna_node_identifier_t *)c->function;
		    if(wcscmp(mgfun->name, L"__memberGet__")==0 && c->child[1]->node_type == ANNA_NODE_IDENTIFIER)
		    {
			anna_node_identifier_t *fun = (anna_node_identifier_t *)c->child[1];
			anna_object_t **stack_object_ptr = anna_stack_addr_get_str(stack, fun->name);
			if(stack_object_ptr)
			{
			    anna_function_t *fun = anna_function_unwrap(*stack_object_ptr);
			    if( fun && (fun->flags & ANNA_FUNCTION_MACRO))
			    {	
				anna_node_t *res = anna_macro_invoke(fun, this2);
				res = anna_node_macro_expand(res, stack);
				return res;
			    }
			}
		    }
		}
	    }
	    
	    this2->function = anna_node_macro_expand(this2->function, stack);
	    
	    if(this2->function->node_type == ANNA_NODE_IDENTIFIER)
	    {
		anna_node_identifier_t *fun = (anna_node_identifier_t *)this2->function;
		anna_object_t **stack_object_ptr = anna_stack_addr_get_str(stack, fun->name);
//		anna_stack_print(stack);
		
		if(stack_object_ptr)
		{
		    anna_function_t *fun2 = anna_function_unwrap(*stack_object_ptr);
		    if( fun2 && (fun2->flags & ANNA_FUNCTION_MACRO))
		    {
						
			anna_node_t *res = anna_macro_invoke(fun2, this2);
			if(!res)
			{
			    anna_error(this, L"Macro expansion resulted in null value");
			    return this;
			}
			res = anna_node_macro_expand(res, stack);
			
			return res;
		    }
		}		
	    }

	    int i;
	    for(i=0;i<this2->child_count;i++)
	    {
		this2->child[i] = anna_node_macro_expand(this2->child[i], stack);
	    }
	    
	    if(this2->function->node_type == ANNA_NODE_MEMBER_GET)
	    {
		anna_node_member_get_t *mg = (anna_node_member_get_t *)this2->function;
		
		anna_node_t *result = (anna_node_t *)anna_node_create_member_call(
		    &this2->location,
		    mg->object,
		    mg->mid,
		    this2->child_count,
		    this2->child);
		return result;
		
	    }

	    return this;
	}
	
	case ANNA_NODE_IDENTIFIER:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_STRING_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_NULL:
	case ANNA_NODE_DUMMY:
	case ANNA_NODE_TYPE_LOOKUP:
	{
	    return this;
	}
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)this;
	    anna_function_t *f = c->payload;
	    
	    if(f->body)
	    {
		int i;
		for(i=0;i<f->body->child_count; i++)
		    f->body->child[i] = anna_node_macro_expand(f->body->child[i], stack);
	    }
	    return this;
	}

	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *c = (anna_node_type_t *)this;
	    anna_type_t *f = c->payload;
	    
	    if(f->definition)
	    {
		anna_node_call_t *body = f->body;
		
		int i;
		for(i=0;i<body->child_count; i++)
		    body->child[i] = anna_node_macro_expand(body->child[i], stack);
	    }
	    return this;
	}

	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_MEMBER_GET_WRAP:
	{
	    anna_node_member_get_t *g = (anna_node_member_get_t *)this;
	    g->object = anna_node_macro_expand(g->object, stack);
	    return this;
	}

	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_set_t *g = (anna_node_member_set_t *)this;
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
	    c->block1 = (anna_node_call_t *)anna_node_macro_expand((anna_node_t *)c->block1, stack);
	    c->block2 = (anna_node_call_t *)anna_node_macro_expand((anna_node_t *)c->block2, stack);
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

int anna_node_is_call_to(anna_node_t *this, wchar_t *name){
    if( this->node_type == ANNA_NODE_CALL)
    {
	anna_node_call_t *this2 = (anna_node_call_t *)this;
	if( this2->function->node_type == ANNA_NODE_IDENTIFIER) 
	{
	    anna_node_identifier_t *fun = (anna_node_identifier_t *)this2->function;
	    return wcscmp(fun->name, name)==0;
	}	
    }
    return 0;
}



void anna_node_calculate_type_param(
    size_t argc,
    anna_node_t **argv,
    int is_method,
    anna_function_type_key_t *funt)
{
    int i, j;
    for(i=0; i<argc; i++)
    {
	if(argv[i]->node_type == ANNA_NODE_CLOSURE)
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)argv[i];
	    anna_function_t *closure = c->payload;
//	    debug(0,L"Closure as param %d. function type says argument is of type %ls\n", i, funt->argv[i+!!is_method]->name);
	    anna_function_type_key_t *template = anna_function_type_extract(funt->argv[i+!!is_method]);
	    assert(template);
//	    debug(0,L"Closure template takes %d params\n", template->argc);
	    for(j=0; j<template->argc; j++)
	    {
//		debug(0,L"Argument %d should be of type %ls\n", j, template->argv[j]->name);
		anna_function_argument_hint(
		    closure,
		    j,
		    template->argv[j]);
	    }
	}
	
    }
}

anna_stack_frame_t *anna_node_register_declarations(
    anna_node_t *this,
    size_t extra)
{
    array_list_t decls = 
	{
	    0,0,0
	}
    ;
    anna_node_find(this, ANNA_NODE_DECLARE, &decls);
    anna_node_find(this, ANNA_NODE_CONST, &decls);
    size_t sz = al_get_count(&decls);
    anna_stack_frame_t *stack = anna_stack_create(sz+extra, 0);    
    int i;
/*
    debug(0,L"WOO WEE WOO %d declarations in ast\n", sz);
    anna_node_print(this);
*/  
    for(i=0; i<sz; i++)
    {
	anna_node_declare_t *decl = al_get(&decls, i);
	if(decl->node_type == ANNA_NODE_DECLARE)
	{
	    anna_stack_declare2(
		stack,
		decl);
	}
	else
	{
	    anna_object_t *value = null_object;
	    switch(decl->value->node_type)
	    {
	
		case ANNA_NODE_TYPE:
		{
		    anna_node_type_t *t = (anna_node_type_t *)decl->value;
		    value = anna_type_wrap(t->payload);
		    break;
		}
		case ANNA_NODE_CLOSURE:
		{
		    anna_node_closure_t *t = (anna_node_closure_t *)decl->value;
		    value = anna_function_wrap(t->payload);
		    break;
		}
		case ANNA_NODE_DUMMY:
		{
		    anna_node_dummy_t *t = (anna_node_dummy_t *)decl->value;
		    value = t->payload;
		    break;
		}
		default:
		{
		    anna_error(
			decl->value,
			L"Constants must have static value\n");
		    break;
		}
	    }
	    
	    anna_stack_declare(
		stack,
		decl->name,
		value->type,
		value,
		0);
	    
	}
	
	anna_sid_t sid = anna_stack_sid_create(stack, decl->name);
	decl->sid = sid;
    }

    //stack_freeze(stack);
    return stack;
}

static anna_type_t *anna_node_resolve_to_type(anna_node_t *node, anna_stack_frame_t *stack)
{
    debug(0,L"Figure out type from:\n");
    
    anna_node_print(node);
    
    if(node->node_type == ANNA_NODE_IDENTIFIER)
    {
	anna_node_identifier_t *id = (anna_node_identifier_t *)node;
	anna_object_t *wrapper = anna_stack_get_str(stack, id->name);
	
	if(wrapper != 0)
	{
	    return anna_type_unwrap(wrapper);
	}
    }
    else if(node->node_type == ANNA_NODE_DUMMY)
    {
	anna_node_dummy_t *d = (anna_node_dummy_t *)node;	
	return anna_type_unwrap(d->payload);
    }
    else if(node->node_type == ANNA_NODE_TYPE_LOOKUP)
    {
	anna_node_type_lookup_t *d = (anna_node_type_lookup_t *)node;	
	anna_node_calculate_type(d->payload, stack);
	if(d->payload->return_type != ANNA_NODE_TYPE_IN_TRANSIT)
	    return d->payload->return_type;
    }
    
    return 0;
}


static void anna_node_calculate_type_internal(
    anna_node_t *this,
    anna_stack_frame_t *stack)
{
    switch(this->node_type)
    {
	
	case ANNA_NODE_INT_LITERAL:
	{
	    this->return_type = int_type;
	    break;
	}
	
	case ANNA_NODE_STRING_LITERAL:
	{
	    this->return_type = string_type;
	    break;
	}
	
	case ANNA_NODE_CHAR_LITERAL:
	{
	    this->return_type = char_type;
	    break;
	}
	
	case ANNA_NODE_FLOAT_LITERAL:
	{
	    this->return_type = float_type;
	    break;
	}

	case ANNA_NODE_IDENTIFIER:
	{
	    anna_node_identifier_t *id = (anna_node_identifier_t *)this;
	    anna_type_t *t = anna_stack_get_type(stack, id->name);
	    
	    if(!t)
	    {
		anna_node_declare_t *decl = anna_stack_get_declaration(stack, id->name);
		if(decl)
		{
		    anna_node_calculate_type((anna_node_t *)decl,stack);
		    if(decl->return_type && decl->return_type != ANNA_NODE_TYPE_IN_TRANSIT)
		    {
			anna_stack_set_type(stack, id->name, decl->return_type);
			t = decl->return_type;
		    }
		}
		
	    }
	    
	    if(!t || t == ANNA_NODE_TYPE_IN_TRANSIT){
		anna_error(this, L"Unknown identifier: %ls", id->name);
//		anna_stack_print(stack);
	    }
	    else
	    {
		this->return_type = t;
	    }
	    
	    break;
	}

	case ANNA_NODE_SPECIALIZE:
	{
	    anna_node_call_t *call = (anna_node_call_t *)this;
	    anna_node_calculate_type(call->function, stack);
	    anna_type_t *type = anna_node_resolve_to_type(call->function, stack);
	    if(!type)
	    {
		anna_error(this, L"Invalid template type");
		break;
	    }
	    if(type == list_type && call->child_count==1)
	    {
		anna_type_t *spec = anna_node_resolve_to_type(call->child[0], stack);
		if(spec)
		{
		    anna_type_t *res = anna_list_type_get(spec);
		    
		    /* FIXME: We remake this node into a new one of a different type- Very, very fugly. Do something prettier, please? */
		    anna_node_dummy_t *new_res = (anna_node_dummy_t *)this;
		    new_res->node_type = ANNA_NODE_DUMMY;
		    new_res->payload = anna_type_wrap(res);
		    new_res->return_type = type_type;
		    break;
		}
	    }
	    anna_error(this, L"Unimplementedtemplate specialization. Come back tomorrow.");
	    break;
	}
		
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *call = (anna_node_call_t *)this;
	    anna_node_calculate_type(call->function, stack);
	    anna_type_t *fun_type = call->function->return_type;

	    if(fun_type == type_type)
	    {
//		debug(0,L"Hmmm, node is of type type...");
//		anna_node_print(call->function);
		
		anna_type_t *type = anna_node_resolve_to_type(call->function, stack);
		if(type)
		{
		    this->node_type = ANNA_NODE_CONSTRUCT;
		    call->function = (anna_node_t *)anna_node_create_dummy(
			&call->function->location,
			anna_type_wrap(type),
			0);
		    call->return_type = type;
		    break;
		}
	    }

	    if(fun_type == ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		break;
	    }

	    anna_function_type_key_t *funt = anna_function_type_extract(fun_type);
	    if(!funt)
	    {
		anna_error(this, L"Value is not callable");
		anna_type_print(fun_type);		
		CRASH;
		break;
	    }
	    
	    if(funt->flags & ANNA_FUNCTION_MACRO)
	    {
		anna_error(this, L"Unexpanded macro call");
		break;
		
	    }
	    
	    call->return_type = funt->result;
	    break;
	}
	
	case ANNA_NODE_MEMBER_CALL:
	{	    
	    anna_node_member_call_t *n = (anna_node_member_call_t *)this;
	    
	    anna_node_calculate_type(n->object, stack);
	    anna_type_t *type = n->object->return_type;
	    if(type == ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		break;
	    }

	    anna_member_t *member = anna_member_get(type, n->mid);
	    
	    if(!member)
	    {
		anna_type_t **types = malloc(sizeof(anna_type_t *)*n->child_count);
		int i;
		int ok = 1;
		
		for(i=0; i<n->child_count; i++)
		{
		    anna_node_calculate_type(n->child[i], stack);
		    types[i] = n->child[i]->return_type;
		    if(type == ANNA_NODE_TYPE_IN_TRANSIT)
		    {
			ok = 0;
			break;
		    }
		}
		if(ok)
		{
		    member = anna_member_method_search(
			type, n->mid, n->child_count, types);
		    
		    if(member)
		    {
			n->mid = anna_mid_get(member->name);
		    }
		}
		
	    }
	    
	    if(!member)
	    {
		anna_error(
		    this, 
		    L"No member named %ls in type %ls\n", 
		    anna_mid_get_reverse(n->mid),
		    type->name);
		break;
	    }

	    anna_function_type_key_t *funt = anna_function_type_extract(member->type);
	    n->return_type = funt->result;

	    anna_node_calculate_type_param(n->child_count, n->child, 1, funt);
	    
	    break;
	}
	
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)this;
//	    debug(0,L"AAA1 %ls\n", c->payload->name);
	    
	    anna_function_setup_interface(c->payload, stack);
//	    debug(0,L"AAA2 %ls\n", c->payload->name);
	    if(c->payload->wrapper)
	    {
		c->return_type = c->payload->wrapper->type;
	    }
//	    debug(0,L"AAA3 %ls\n", c->payload->name);
//	    anna_function_setup_body(c->payload, stack);
//	    debug(0,L"AAA4 %ls\n", c->payload->name);
	    
	    break;
	}
	
	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *c = (anna_node_type_t *)this;
	    anna_type_t *f = c->payload;
	    
	    if(f->definition)
	    {
		anna_type_setup_interface(f, stack);
		c->return_type = type_type;
	    }
	    return this;
	}

	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *c = (anna_node_assign_t *)this;
	    anna_node_calculate_type(c->value, stack);
	    c->return_type = c->value->return_type;
	    break;
	}

	case ANNA_NODE_NULL:
	{
	    this->return_type = null_type;
	    break;
	}

	case ANNA_NODE_MEMBER_GET:
	{
	    anna_node_member_get_t *c = (anna_node_member_get_t *)this;
	    anna_node_calculate_type(c->object, stack);
	    anna_type_t *type = c->object->return_type;
	    if(type == ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		break;
	    }
	    
	    anna_member_t *member = anna_member_get(type, c->mid);
	    if(!member)
	    {
		anna_error(
		    this, 
		    L"No member named %ls in type %ls\n", 
		    anna_mid_get_reverse(c->mid),
		    type->name);
		break;
		
	    }
	    
	    if(member->is_method)
	    {
		c->node_type = ANNA_NODE_MEMBER_GET_WRAP;
	    }
	    c->return_type = member->type;
	    break;
	}

	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_set_t *g = (anna_node_member_set_t *)this;
	    anna_node_calculate_type(g->value, stack);
	    g->return_type = g->value->return_type;
	    break;
	}
	
	case ANNA_NODE_DECLARE:
	case ANNA_NODE_CONST:
	{
	    anna_node_declare_t *d = (anna_node_declare_t *)this;
	    if(d->type->node_type == ANNA_NODE_IDENTIFIER)
	    {
		anna_node_identifier_t *t = node_cast_identifier(d->type);
		anna_object_t *t2 = anna_stack_get_str(stack, t->name);	    
		d->return_type = anna_type_unwrap(t2);
	    }
	    else if(d->type->node_type == ANNA_NODE_NULL)
	    {
		if(d->value->node_type == ANNA_NODE_NULL)
		{
		    anna_error(this, L"No type specified for variable declaration");
		}
		anna_node_calculate_type(d->value, stack);
		d->return_type = d->value->return_type;
	    }
	    else
	    {
		anna_error(d->type, L"Invalid type for declaration");
	    }
	    if(d->return_type != ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		anna_stack_set_type(stack, d->name, d->return_type);
	    }
	    
	    break;
	}
	
	case ANNA_NODE_OR:
	{
	    anna_node_cond_t *d = (anna_node_cond_t *)this;
	    anna_node_calculate_type(d->arg1, stack);
	    anna_node_calculate_type(d->arg2, stack);
	    if((d->arg1->return_type == ANNA_NODE_TYPE_IN_TRANSIT) ||
	       (d->arg2->return_type == ANNA_NODE_TYPE_IN_TRANSIT))
	    {
		break;
	    }
	    
	    d->return_type = anna_type_intersect(
		d->arg1->return_type,
		d->arg2->return_type);
	    
	    break;   
	}

	case ANNA_NODE_AND:
	{
	    anna_node_cond_t *d = (anna_node_cond_t *)this;
	    anna_node_calculate_type(d->arg2, stack);
	    d->return_type = d->arg2->return_type;
	    break;
	}
	
	case ANNA_NODE_WHILE:
	{
	    anna_node_cond_t *d = (anna_node_cond_t *)this;
	    anna_node_calculate_type(d->arg2, stack);
	    d->return_type = d->arg2->return_type;
	    break;
	}
	
	case ANNA_NODE_IF:
	{
	    anna_node_if_t *d = (anna_node_if_t *)this;

	    anna_node_calculate_type((anna_node_t *)d->block1, stack);
	    anna_node_calculate_type((anna_node_t *)d->block2, stack);
	    if((d->block1->return_type == ANNA_NODE_TYPE_IN_TRANSIT) ||
	       (d->block2->return_type == ANNA_NODE_TYPE_IN_TRANSIT))
	    {
		break;
	    }
	    if(
		(d->block1->node_type != ANNA_NODE_CLOSURE) || 
		(d->block1->node_type != ANNA_NODE_CLOSURE))
	    {
		anna_error(this, L"Parameters to if expression must be closures");
		break;
	    }
	    anna_node_closure_t *b1 = (anna_node_closure_t *)d->block1;
	    anna_node_closure_t *b2 = (anna_node_closure_t *)d->block2;
	    
	    d->return_type = anna_type_intersect(
		b1->payload->return_type,
		b2->payload->return_type);
	    
	    break;
	}	
	
	case ANNA_NODE_DUMMY:
	{
	    anna_node_dummy_t *d = (anna_node_dummy_t *)this;
	    d->return_type = d->payload->type;
	    break;   
	}
	
	default:
	{
	    anna_error(
		this,
		L"Don't know how to handle node of type %d during type calculation", this->node_type);	    
	    break;
	}
    }
}

void anna_node_prepare_body(
    anna_node_t *this)
{
    switch(this->node_type)
    {
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)this;    
	    anna_function_setup_body(c->payload);
	    break;
	}
    }
}


void anna_node_calculate_type(
    anna_node_t *this,
    anna_stack_frame_t *stack)
{
    if(this->return_type == ANNA_NODE_TYPE_IN_TRANSIT)
    {
	anna_error(this, L"Circular type checking dependency");
	anna_node_print(this);
    }
    else if(this->return_type == 0)
    {
	this->return_type = ANNA_NODE_TYPE_IN_TRANSIT;
	anna_node_calculate_type_internal( this, stack );
    }
}



