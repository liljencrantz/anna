
static anna_type_t *anna_method_curry(anna_function_type_t *fun)
{
    anna_function_t *res = anna_native_create(
	anna_util_identifier_generate(L"!anonymousFunction", 0),
	fun->flags,
	(anna_native_t)anna_vm_null_function,
	fun->return_type,
	fun->input_count-1,
	fun->input_type+1,
	fun->input_name+1,
	0);
    
    return res->wrapper->type;
}

static void try_hint(
    anna_function_t *closure,
    anna_function_type_t *template)
{    
    int j;
    if(template)
    {
	for(j=0; j<template->input_count; j++)
	{
	    anna_function_argument_hint(
		closure,
		j,
		template->input_type[j]);
	}
    }
}

int blabla = 0;


static void anna_node_calculate_type_param(
    size_t argc,
    anna_node_t **argv,
    int is_method,
    anna_function_type_t *funt)
{
    return;
    int i;
    for(i=0; i<argc; i++)
    {	
	anna_function_type_t *template = anna_function_type_unwrap(funt->input_type[i+!!is_method]);

	if(template && (argv[i]->node_type != ANNA_NODE_CLOSURE))
	{
	    wprintf(L"Trying to give a hint to\n");
	    anna_node_print(5, argv[i]);
	}
	
	if(argv[i]->node_type == ANNA_NODE_CLOSURE)
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)argv[i];
	    try_hint(c->payload, template);
	}
	else if(argv[i]->node_type == ANNA_NODE_IDENTIFIER)
	{
	    anna_object_t *fun_obj = anna_node_static_invoke_try(
		argv[i],
		argv[i]->stack);
	    if(blabla)
	    {
		wprintf(L"%d\n", fun_obj);
		CRASH;
	    }	

	    if(fun_obj)
	    {
		anna_function_t *closure = anna_function_unwrap(fun_obj);
		if(closure)
		{
		    try_hint(closure, template);
		}		
	    }	    
	}
    }
}

static void anna_node_set_stack_fun(anna_node_t *node, void *stack_ptr)
{
//    wprintf(L"Set stack %d for node %d of type %d\n", stack_ptr, node, node->node_type);
    node->stack = (anna_stack_template_t *)stack_ptr;
    switch(node->node_type)
    {
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)node;
//	    wprintf(L"Set stack %d for closure %ls\n", stack_ptr, c->payload->name);
	    anna_function_set_stack(
		c->payload,
		(anna_stack_template_t *)stack_ptr);
	    break;
	}
	
	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *c = (anna_node_type_t *)node;
	    anna_type_set_stack(
		c->payload,
		(anna_stack_template_t *)stack_ptr);	    
	    break;
	}
	
    }
    
}

void anna_node_set_stack(
    anna_node_t *this,
    anna_stack_template_t *stack)
{
    anna_node_each(
	this, anna_node_set_stack_fun, stack);
}

void anna_node_register_declarations(
    anna_node_t *this,
    anna_stack_template_t *stack)
{
    array_list_t decls = AL_STATIC;

    anna_node_find(this, ANNA_NODE_DECLARE, &decls);
    anna_node_find(this, ANNA_NODE_CONST, &decls);
    size_t sz = al_get_count(&decls);
    int i;
    
    for(i=0; i<sz; i++)
    {
	anna_node_declare_t *decl = al_get(&decls, i);
	anna_stack_declare2(
	    stack,
	    decl);
    }
    al_destroy(&decls);   
}


/**
   Calculate the types of all direct child nodes of a call node,
   without also calculating the type of any other grandchildren except
   as needed. This is useful e.g. when using parameter types to pick
   one of multiple aliased methods.
 */
static int anna_node_calculate_type_direct_children(anna_node_call_t *n, anna_stack_template_t *stack)
{
    int i;
    
    for(i=0; i<n->child_count; i++)
    {
	anna_node_calculate_type(n->child[i]);
	if(n->child[i]->return_type == ANNA_NODE_TYPE_IN_TRANSIT)
	{
	    return 0;
	}
    }
    return 1;
    
}

static void anna_node_calculate_type_internal(
    anna_node_t *this)
{
    anna_stack_template_t *stack = this->stack;

    switch(this->node_type)
    {
	
	case ANNA_NODE_INT_LITERAL:
	{
	    this->return_type = int_type;
	    break;
	}
	
	case ANNA_NODE_MAPPING_IDENTIFIER:
	{
	    this->return_type = null_type;
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
//	    anna_node_print(6,id);
	    anna_type_t *t = anna_stack_get_type(stack, id->name);
	    
	    if(!t)
	    {
		anna_node_declare_t *decl = anna_stack_get_declaration(stack, id->name);
		
		if(decl)
		{
		    anna_node_calculate_type((anna_node_t *)decl);
		    if(decl->return_type && decl->return_type != ANNA_NODE_TYPE_IN_TRANSIT)
		    {
			anna_stack_set_type(stack, id->name, decl->return_type);
			t = decl->return_type;
		    }
		}
	    }
	    if(t == null_type)
	    {
		anna_error(this, L"Invalid type for variable %ls", id->name);
		CRASH;
		break;
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
	    anna_node_specialize(call, stack);
	    
	    break;
	}
		
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *call = (anna_node_call_t *)this;

	    anna_node_calculate_type(call->function);
	    anna_type_t *fun_type = call->function->return_type;
	    
	    if(fun_type == type_type)
	    {
//		debug(D_SPAM,L"Hmmm, node is of type type...");
//		anna_node_print(0, call->function);
		
		anna_type_t *type = anna_node_resolve_to_type(call->function, stack);
		if(type)
		{
		    if(!anna_node_calculate_type_direct_children(call, stack))
		    {
			break;
		    }
		    
		    type = anna_type_implicit_specialize(type, call);
		    this->node_type = ANNA_NODE_CONSTRUCT;
		    call->function = (anna_node_t *)anna_node_create_type(
			&call->function->location,
			type);
		    call->function->stack = call->stack;
		    call->return_type = type;
		    break;
		}
		
	    }
	    
	    if(fun_type == ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		break;
	    }

	    anna_function_type_t *funt = anna_function_type_unwrap(fun_type);
	    if(!funt)
	    {
//		anna_node_print(4, call->function);
		anna_error(call->function, L"Value of type %ls is not callable", fun_type->name);
		break;
	    }
	    
	    if(funt->flags & ANNA_FUNCTION_MACRO)
	    {
		anna_error(call->function, L"Found unexpanded macro call while calculating function return type");
		
		break;
	    }
	    
	    if(anna_node_call_validate(call, funt, 0, 1))
	    {
		anna_node_call_map(call, funt, 0);		
	    }

	    
	    call->return_type = funt->return_type;
	    break;
	}

	case ANNA_NODE_CAST:
	{
	    anna_node_call_t *call = (anna_node_call_t *)this;
	    /*
	      First calculate the type of the thing we are casting. We
	      do this because e.g. the map macro uses the type of the
	      castee when determining the type to cast to. This is a
	      bit of a fragile hack, we should probably figure out
	      something more robust.
	      
	      Perhaps it would be possible for the type lookup nodes
	      to be given a manual node which, if specified, needs to
	      be type calculated before they are calculated?
	    */
	    anna_node_calculate_type(call->child[0]);
	    anna_node_calculate_type(call->child[1]);
	    anna_type_t *fun_type = call->child[1]->return_type;

	    if(fun_type == type_type)
	    {
		anna_type_t *type = anna_node_resolve_to_type(call->child[1], stack);
		if(type)
		{
		    call->return_type = type;
		    break;
		}
	    }

	    if(fun_type == ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		break;
	    }

	    break;
	}
	
	case ANNA_NODE_MEMBER_CALL:
	{	    
	    anna_node_call_t *n = (anna_node_call_t *)this;
	    
	    anna_node_calculate_type(n->object);
	    anna_type_t *type = n->object->return_type;
	    if(type == ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		break;
	    }	    
	    
	    if(type == null_type)
	    {
		anna_error(n->object, L"Invalid type for object in call");
		break;
	    }

	    anna_type_prepare_member(type, n->mid, stack);
	    
	    array_list_t method_candidate = AL_STATIC;
	    anna_member_t *member = anna_member_get(type, n->mid);
	    
	    if(!member)
	    {
		
		int ok = anna_node_calculate_type_direct_children(n, stack);
		
		if(ok)
		{
		    member = anna_member_method_search(
			type, n->mid, n, 0);
		    
		    if(member)
		    {
			n->mid = anna_mid_get(member->name);
		    }
		    else
		    {
			if(n->child_count == 1)
			{
			    anna_node_call_t *n2 = (anna_node_call_t *)anna_node_clone_shallow((anna_node_t *)n);
			    anna_node_t *tmp = n2->object;
			    n2->object = n2->child[0];
			    n2->child[0] = tmp;
			    
			    member = anna_member_method_search(
				n->child[0]->return_type, n->mid, n2, 1);
			    if(member)
			    {
				/*
				  Reverse method alias. We can safely
				  switch the pointers around, we just
				  calculated the types of all involved
				  nodes.
				*/
				tmp = n->object;
				n->object = n->child[0];
				n->child[0] = tmp;
				n->mid = anna_mid_get(member->name);
			    }
			}
		    }
		}
	    }
	    
	    if(member)
	    {
		if(member->type == type_type && member->is_static)
		{
//		    debug(4,L"Hmmm, node is of type type...");
//		    anna_node_print(4, n);
		    
		    anna_type_t *ctype = anna_type_unwrap(anna_as_obj(type->static_member[member->offset]));
		    
		    if(ctype)
		    {
			if(!anna_node_calculate_type_direct_children(n, stack))
			{
			    break;
			}

			ctype = anna_type_implicit_specialize(ctype, n);
			this->node_type = ANNA_NODE_CONSTRUCT;
			n->function = (anna_node_t *)anna_node_create_type(
			    &n->object->location,
			    ctype);
			n->function->stack = n->stack;
			n->return_type = ctype;
			break;
		    }
		}

		anna_function_type_t *fun = anna_function_type_unwrap(member->type);
		if(!fun)
		{
		    anna_error(
			this, 
			L"Member %ls is not a function\n", 
			anna_mid_get_reverse(n->mid),
			type->name);		    
		    break;
		}
		
		if(!anna_node_call_validate(n, fun, member->is_method, 1))
		{
		    member = 0;
		}
		else
		{
		    anna_node_call_map(n, fun, member->is_method);
		    
		}
	    }
	    else
	    {
		anna_error(
		    this, 
		    L"No member named %ls in type %ls\n", 
		    anna_mid_get_reverse(n->mid),
		    type->name);
		break;
	    }

	    if(member)
	    {
		anna_function_type_t *funt = anna_function_type_unwrap(member->type);
		n->return_type = funt->return_type;
		if(n->mid == anna_mid_get(L"__map__"))
		    blabla=1;
		
	    
		anna_node_calculate_type_param(n->child_count, n->child, 1, funt);
	    }
	    
	    break;
	}
	
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)this;
	    
	    anna_function_setup_interface(c->payload);
	    if(c->payload->wrapper)
	    {
		c->return_type = c->payload->wrapper->type;
	    }
//	    anna_function_setup_body(c->payload);	    
	    break;
	}
	
	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *c = (anna_node_type_t *)this;
	    anna_type_t *f = c->payload;
	    c->return_type = type_type;	    
	    anna_type_set_stack(f, stack);
	    anna_type_setup_interface(f);

	    break;
	}

	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *c = (anna_node_assign_t *)this;
	    anna_node_calculate_type(c->value);
	    c->return_type = c->value->return_type;
	    break;
	}

	case ANNA_NODE_NULL:
	{
	    this->return_type = null_type;
	    break;
	}

	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_access_t *c = (anna_node_member_access_t *)this;
	    anna_node_calculate_type(c->object);
	    anna_type_t *type = c->object->return_type;
	    if(type == ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		break;
	    }
	    
	    anna_type_prepare_member(type, c->mid, stack);
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

	    if(c->node_type == ANNA_NODE_MEMBER_GET)
	    {
		if(member->is_method)
		{
		    c->node_type = ANNA_NODE_MEMBER_BIND;
		    c->return_type = anna_method_curry(anna_function_type_unwrap(member->type));
		}
		else
		{
		    c->return_type = member->type;
		}
	    }
	    else
	    {
		anna_node_calculate_type(c->value);
		c->return_type = c->value->return_type;
		
	    }
	    
	    break;
	}

	case ANNA_NODE_CONST:
	case ANNA_NODE_DECLARE:
	{
	    anna_node_declare_t *d = (anna_node_declare_t *)this;
//	    debug(D_ERROR, L"Calculating type of declaration %ls\n", d->name);
	    if(d->type->node_type == ANNA_NODE_NULL)
	    {
//		debug(D_ERROR, L"Declaration %ls has implicit type\n", d->name);
		if(d->value->node_type == ANNA_NODE_NULL)
		{
		    anna_error(this, L"No type specified for variable declaration\n");
		}
		anna_node_calculate_type(d->value);
		d->return_type = d->value->return_type;
		if(d->return_type != ANNA_NODE_TYPE_IN_TRANSIT)
		{
		    anna_stack_set_type(stack, d->name, d->return_type);
		}
	    }
	    else
	    {
//		debug(D_ERROR, L"Declaration %ls has explicit type\n", d->name);
		d->return_type = anna_node_resolve_to_type(
		    d->type,
		    stack);
		if(!d->return_type)
		{
		    anna_error(d->type, L"Invalid type for declaration");
		    d->return_type = ANNA_NODE_TYPE_IN_TRANSIT;
		    CRASH;
		}
	    }
	    
	    if(this->node_type == ANNA_NODE_CONST)
	    {
//		debug(D_ERROR, L"Declaration %ls is a constant\n", d->name);
		anna_object_t *value = anna_node_static_invoke(
		    d->value, stack);
		anna_stack_set(
		    stack,
		    d->name,
		    value);
		anna_stack_set_flag(
		    stack,
		    d->name,
		    ANNA_STACK_READONLY);
	    }
	    
//	    debug(D_ERROR, L"Type calculation of declaration %ls finished\n", d->name);
	    break;
	}
	
	case ANNA_NODE_OR:
	{
	    anna_node_cond_t *d = (anna_node_cond_t *)this;
	    anna_node_calculate_type(d->arg1);
	    anna_node_calculate_type(d->arg2);
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
	case ANNA_NODE_MAPPING:
	{
	    anna_node_cond_t *d = (anna_node_cond_t *)this;
	    anna_node_calculate_type(d->arg1);
	    anna_node_calculate_type(d->arg2);
	    d->return_type = d->arg2->return_type;
	    break;
	}
	
	case ANNA_NODE_WHILE:
	{
	    anna_node_cond_t *d = (anna_node_cond_t *)this;
	    anna_node_calculate_type(d->arg2);
	    anna_type_t *fun_type =  d->arg2->return_type;
	    if(fun_type == ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		break;
	    }
	    anna_function_type_t *funt = anna_function_type_unwrap(fun_type);
	    if(!funt)
	    {
		anna_error(this, L"Value is not callable");
		break;
	    }
	    if(funt->flags & ANNA_FUNCTION_MACRO)
	    {
		anna_error(this, L"Unexpanded macro call");
		break;
	    }
	    
	    d->return_type = funt->return_type;

	    break;
	}
	
	case ANNA_NODE_IF:
	{
	    anna_node_if_t *d = (anna_node_if_t *)this;

	    anna_node_calculate_type((anna_node_t *)d->block1);
	    anna_node_calculate_type((anna_node_t *)d->block2);
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
	
	case ANNA_NODE_RETURN:
	{
	    anna_node_wrapper_t *c = (anna_node_wrapper_t *)this;
	    anna_node_calculate_type(c->payload);
	    c->return_type = c->payload->return_type;
	    break;
	}

	case ANNA_NODE_TYPE_OF:
	{	    
	    anna_node_t *chld = anna_node_type_lookup_get_payload(this);
	    if(!chld->stack)
		CRASH;
	    
	    anna_node_calculate_type(chld);
	    this->return_type = chld->return_type;
	    break;
	}
	
	case ANNA_NODE_RETURN_TYPE_OF:
	{
	    anna_node_t *chld = anna_node_type_lookup_get_payload(this);
	    if(!chld->stack)
		CRASH;
	    
	    anna_node_calculate_type(chld);

	    if(chld->return_type != ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		anna_function_type_t *fun = anna_function_type_unwrap(
		   chld->return_type);
		if(fun)
		{
		    this->return_type = fun->return_type;
		}
	    }
	    break;
	}
	
	case ANNA_NODE_INPUT_TYPE_OF:
	{
//	    wprintf(L"GGGG, input type thingie %ld\n", this);
//	    anna_node_print(5, this);
	    
	    anna_node_wrapper_t *c = (anna_node_wrapper_t *)this;
	    anna_node_t *chld = anna_node_type_lookup_get_payload(this);
	    if(!chld->stack)
		CRASH;
	    
	    anna_node_calculate_type(chld);
	    
	    if(chld->return_type != ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		anna_function_type_t *fun = anna_function_type_unwrap(
		   chld->return_type);
//		wprintf(L"Wee %ls\n", chld->return_type->name);
		if(fun && fun->input_count > c->steps)
		{
		    this->return_type = fun->input_type[c->steps];
//		    wprintf(L"Woo WEEE woo, input type thingie is %ls\n", this->return_type->name);
		    
		}
	    }
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
    anna_node_t *this)
{

    if(!this->stack)
    {
	wprintf(L"FAFASD %d %d\n", this->node_type, this);
	anna_error(this,L"Invalid stack value while determining types\n");
	CRASH;
    }
    
    anna_stack_template_t *stack = this->stack;
    
//    debug(D_CRITICAL, L"Calculate type of node:\n");
//    anna_node_print(D_CRITICAL, this);
    if(this->return_type == ANNA_NODE_TYPE_IN_TRANSIT && anna_error_count == 0)
    {
	anna_error(this, L"Circular type checking dependency");
	anna_node_print(0, this);
    }
    else if(this->return_type == 0)
    {
	this->return_type = ANNA_NODE_TYPE_IN_TRANSIT;

	int i;
	for(i=0; i<al_get_count(&this->dependencies); i++)
	{
	    anna_node_t *dep = (anna_node_t *)al_get(&this->dependencies, i);
	    anna_node_calculate_type(dep);
	}

	anna_node_calculate_type_internal( this );
    }
    debug(D_SPAM, L"Done\n");
}

void anna_node_validate(anna_node_t *this, anna_stack_template_t *stack)
{
    switch(this->node_type)
    {
	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_access_t *c = (anna_node_member_access_t *)this;
	    anna_member_t *memb = anna_member_get(c->object->return_type, c->mid);
	    if(memb->is_property && memb->setter_offset == -1)
	    {
		anna_error(this, L"No setter for property %ls", anna_mid_get_reverse(c->mid));
		break;		
	    }
	    
	    break;
	}
	
	case ANNA_NODE_MEMBER_GET:
	{
	    anna_node_member_access_t *c = (anna_node_member_access_t *)this;
	    anna_member_t *memb = anna_member_get(c->object->return_type, c->mid);
	    if(memb->is_property && memb->getter_offset == -1)
	    {
		anna_error(this, L"No getter for property %ls", anna_mid_get_reverse(c->mid));
		break;		
	    }
	    
	    break;
	}
	
	case ANNA_NODE_MEMBER_CALL:
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_CALL:
	{
	    anna_function_type_t *ftk=0;
	    anna_type_t **tmpl;
	    int tmpl_count;
	    anna_node_call_t *this2 =(anna_node_call_t *)this;

	    if(this->node_type == ANNA_NODE_CONSTRUCT)
	    {
		anna_type_t *ft = this2->function->return_type;
		if(!ft)
		{
		    anna_error(this, L"Invalid return type");
		    break;
		}
	    
		anna_node_type_t *tn = (anna_node_type_t *)this2->function;
		
		anna_entry_t **constructor_ptr = anna_entry_get_addr_static(
		    tn->payload,
		    ANNA_MID_INIT_PAYLOAD);
		assert(constructor_ptr);
		ftk = anna_function_type_unwrap(
		    anna_as_obj(*constructor_ptr)->type);
		if(ftk)
		{
		    tmpl = ftk->input_type+1;
		    tmpl_count = ftk->input_count-1;
		}
		
	    }
	    else if(this->node_type == ANNA_NODE_MEMBER_CALL)
	    {
		anna_member_t *memb = 
		    anna_member_get(this2->object->return_type, this2->mid);
		
		anna_type_t *ft = memb->type;
		
		ftk = anna_function_type_unwrap(ft);	    
		
		if(ftk)
		{
		    tmpl = ftk->input_type;
		    tmpl_count = ftk->input_count;
		    if(memb->is_method)
		    {
			tmpl++;
			tmpl_count--;
		    }
		}
	    }
	    else
	    {
		anna_type_t *ft = this2->function->return_type;
		if(!ft)
		{
		    anna_error(this, L"Invalid return type");
		    break;
		}
		
		ftk = anna_function_type_unwrap(ft);
		
		if(ftk)
		{
		    tmpl = ftk->input_type;
		    tmpl_count = ftk->input_count;
		}
	    }
	    
	    if(!ftk)
	    {
		anna_error(this, L"Tried to call a non-function");
		break;
	    }
	    if(ftk->flags & ANNA_FUNCTION_VARIADIC)
	    {
		if( this2->child_count < tmpl_count-1)
		{
		    anna_error(
			this,
			L"Too few parameters to function call. Expected at least %d, got %d\n", 
			tmpl_count-1, this2->child_count);
		    break;
		}
	    }
	    else
	    {
		if(tmpl_count != this2->child_count)
		{
		    anna_error(
			this,
			L"Wrong number of parameters to function call. Expected %d, got %d\n", 
			tmpl_count, this2->child_count);
		    anna_node_print(D_ERROR, this);
		    break;
		}
	    }
	    int i;
	    for(i=0; i<this2->child_count; i++)
	    {
		anna_type_t *param = this2->child[i]->return_type;
		anna_type_t *templ = tmpl[mini(i, tmpl_count-1)];
		if(!anna_abides(param, templ))
		{
		    
		    anna_error(
			this,
			L"Invalid type of parameter %d in function call. Expected type %ls, got type %ls",
			i+1, templ->name, param->name);
		}
		
	    }
	    	    
	    break;	    
	}

	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *d = (anna_node_assign_t *)this;
	    anna_type_t *param = d->value->return_type;
	    anna_type_t *templ = anna_stack_get_type(stack, d->name);
	    if(!templ)
	    {
		anna_error(
		    this,
		    L"Unknown identifier: %ls",
		    d->name);
//		    anna_node_print(D_ERROR,this);
	    }
	    else if(!anna_abides(param, templ))
	    {
		anna_error(
		    this,
		    L"Invalid type in assignment. Expected argument of type %ls, but supplied value of type %ls does not qualify.", 
		    templ->name, param->name);
//		    anna_node_print(D_ERROR,this);
	    }
	    
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
		{
		    anna_node_each(f->body->child[i], (anna_node_function_t)&anna_node_validate, f->stack_template);
		}
	    }
	    break;
	}	
    }    
}

static void anna_node_calculate_type_fun(
    anna_node_t *this,
    void *aux)
{
    anna_node_calculate_type(this);
}

void anna_node_calculate_type_children(anna_node_call_t *node)
{
    int i;
    for(i=0; i<node->child_count; i++)
    {
	anna_node_each(node->child[i], (anna_node_function_t)&anna_node_calculate_type_fun, 0);
	if(anna_error_count)
	{
	    return;
	}
    }
    anna_node_each((anna_node_t *)node, (anna_node_function_t)&anna_node_prepare_body, 0);
}
