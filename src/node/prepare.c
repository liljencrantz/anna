static anna_node_t *resolve_identifiers_each(
    anna_node_t *this, void *aux);

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
	fun->input_default+1,
	0);
    
    return res->wrapper->type;
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
//	    wprintf(L"\nSet stack %d for closure %ls\n", stack_ptr, c->payload->name);
	    anna_function_set_stack(
		c->payload,
		(anna_stack_template_t *)stack_ptr);
	    break;
	}
	
	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *c = (anna_node_type_t *)node;
//	    wprintf(L"\nSet stack %d for type %ls\n", stack_ptr, c->payload->name);
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
static int anna_node_calculate_type_direct_children(
    anna_node_call_t *n, anna_stack_template_t *stack)
{
    int i;
    
    for(i=0; i<n->child_count; i++)
    {
	n->child[i] = anna_node_calculate_type(n->child[i]);
	if(n->child[i]->return_type == ANNA_NODE_TYPE_IN_TRANSIT)
	{
	    return 0;
	}
    }
    return 1;
    
}

static anna_node_t *anna_node_calculate_type_internal_call(
    anna_node_call_t *n)
{
    int is_constructor = 0;
    anna_stack_template_t *stack = n->stack;
	    
    n->object = anna_node_calculate_type(n->object);
    anna_type_t *type = n->object->return_type;
	    
    if(type == ANNA_NODE_TYPE_IN_TRANSIT)
    {
	return (anna_node_t *)n;
    }
	    
    if(type == null_type)
    {
	anna_error(n->object, L"Invalid type for object in call");
	return (anna_node_t *)n;
    }
	    
    anna_type_set_stack(type, n->stack);
    anna_type_setup_interface(type);

    if(n->node_type == ANNA_NODE_STATIC_MEMBER_CALL)
    {
	type = anna_node_resolve_to_type(n->object, stack);
	n->access_type = ANNA_NODE_ACCESS_STATIC_MEMBER;

	if(!type)
	{
	    anna_error(n->object, L"Unknown type");
	    return (anna_node_t *)n;
	}
    }
    
    anna_type_prepare_member(type, n->mid, stack);	    
    anna_member_t *member = anna_member_get(type, n->mid);
    
    if(!member)
    {		
	if(anna_node_calculate_type_direct_children(n, stack))
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
		    anna_node_call_t *n2 = (anna_node_call_t *)anna_node_clone_shallow(
			(anna_node_t *)n);
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
	if(member->type == type_type && anna_member_is_static(member))
	{
//		    debug(4,L"Hmmm, node is of type type...");
//		    anna_node_print(4, n);
	    
	    anna_type_t *ctype = anna_type_unwrap(
		anna_as_obj(
		    type->static_member[member->offset]));
		    
	    if(ctype)
	    {
		if(!anna_node_calculate_type_direct_children(n, stack))
		{
		    return (anna_node_t *)n;
		}

		ctype = anna_type_implicit_specialize(ctype, n);
		n->node_type = ANNA_NODE_CONSTRUCT;
		n->function = (anna_node_t *)anna_node_create_type(
		    &n->object->location,
		    ctype);
		n->function->stack = n->stack;
		n->return_type = ctype;
		
		member = anna_member_get(ctype, anna_mid_get(L"__init__"));
		is_constructor = 1;
	    }
	}
	
	anna_function_type_t *fun = anna_function_type_unwrap(member->type);
	if(!fun)
	{
	    anna_error(
		(anna_node_t *)n,
		L"Member %ls is not a function\n",
		anna_mid_get_reverse(n->mid),
		type->name);
	    return (anna_node_t *)n;
	}
		
	if(!anna_node_validate_call_parameters(
	       n, fun,
	       anna_member_is_bound(member) && !(n->access_type == ANNA_NODE_ACCESS_STATIC_MEMBER),
	       1))
	{
	    member = 0;
	}
	else
	{
	    anna_node_call_map(n, fun, anna_member_is_bound(member));
	}
    }
    else
    {
	anna_error(
	    (anna_node_t *)n, 
	    L"No member named %ls in type %ls\n", 
	    anna_mid_get_reverse(n->mid),
	    type->name);
	return (anna_node_t *)n;
    }

    if(!is_constructor && member)
    {
	anna_function_type_t *funt = anna_function_type_unwrap(member->type);
	n->return_type = funt->return_type;
    }
    return (anna_node_t *)n;
}

static void anna_function_search_internal(
    anna_stack_template_t *stack, wchar_t *alias, array_list_t *stack_decl, array_list_t *use_memb)
{
    if(!stack)
    {
	return;
    }
    
    int i, j;
    for(i=0; i<stack->count; i++)
    {
	anna_node_declare_t *decl = stack->member_declare_node[i];
	if(
	    decl && 
	    decl->attribute && 
	    anna_attribute_has_alias(
		decl->attribute,
		alias))
	{
	    
	    stack->member_declare_node[i] = decl = 
		(anna_node_declare_t *)anna_node_calculate_type(
		    (anna_node_t *)decl);
	    if(decl->return_type && (decl->return_type != ANNA_NODE_TYPE_IN_TRANSIT))
	    {
		if(anna_function_type_unwrap(decl->return_type))
		{
		    al_push(stack_decl, decl);
		}
	    }
	}
    }

    anna_function_search_internal(
	stack->parent, alias, stack_decl, use_memb);

    for(j=0; j<al_get_count(&stack->import); j++)
    {
	anna_use_t *use = al_get(&stack->import, j);
	
	
	array_list_t *memb_list = &use->type->member_list;
	for(i=0; i<al_get_count(memb_list); i++)
	{
	    anna_member_t *memb = al_get(memb_list, i);
	    if(anna_member_is_static(memb) && memb->offset>=0 && memb->type != null_type)
	    {	    
		anna_object_t *memb_val = anna_as_obj(use->type->static_member[memb->offset]);
		anna_function_t *memb_fun = anna_function_unwrap(memb_val);
	    
		if(
		    memb_fun &&
		    memb_fun->attribute && 
		    anna_attribute_has_alias(
			memb_fun->attribute,
			alias))
		{
		    al_push(use_memb, use);
		    al_push(use_memb, memb);
		}
	    }
	}
    }
    
}

static wchar_t *anna_function_search(
    anna_stack_template_t *stack, wchar_t *alias, anna_node_call_t *call)
{
    wchar_t *res = 0;
    array_list_t stack_decl = AL_STATIC;
    array_list_t use_memb = AL_STATIC;
    anna_function_search_internal(
	stack, alias, &stack_decl, &use_memb);
    int i;
    size_t count=0;
    
    if(al_get_count(&stack_decl) || al_get_count(&use_memb))
    {
	anna_function_type_t **ft = 
	    malloc(sizeof(anna_function_type_t *)*(al_get_count(&stack_decl) + al_get_count(&use_memb)));
	wchar_t **name = 
	    malloc(sizeof(wchar_t *)*(al_get_count(&stack_decl)+al_get_count(&use_memb)));
	
	for(i=0; i<al_get_count(&stack_decl); i++)
	{
	    anna_node_declare_t *decl = (anna_node_declare_t *)al_get(&stack_decl, i);
	    if(anna_stack_get_declaration(stack, decl->name) == decl)
	    {
		name[count] = decl->name;
		ft[count++] = anna_function_type_unwrap(decl->return_type);
	    }
	}
	while(al_get_count(&use_memb))
	{
	    anna_member_t *memb = (anna_member_t *)al_pop(&use_memb);
	    anna_use_t *use = (anna_member_t *)al_pop(&use_memb);
	    
	    if(anna_stack_search_use(stack, memb->name) == use)
	    {
		name[count] = memb->name;
		ft[count++] = anna_member_bound_function_type(memb);
	    }
	}
	
	
	if(count)
	{
	    int idx = anna_abides_search(
		call, ft, count);
	    if(idx != -1)
	    {
		res = name[idx];
	    }
	}
	
	free(ft);
	free(name);	
    }
    
    al_destroy(&stack_decl);
    al_destroy(&use_memb);
    return res;
}

static anna_node_t *anna_node_calculate_type_internal(
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
	
	case ANNA_NODE_INTERNAL_IDENTIFIER:
	{
	    this->return_type = null_type;
	    break;
	}
	
	case ANNA_NODE_STRING_LITERAL:
	{
	    this->return_type = imutable_string_type;
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
	    
	    anna_module_check(stack_global, id->name);
	    
	    anna_type_t *t = anna_stack_get_type(stack, id->name);
	    
	    if(!t)
	    {
		anna_node_identifier_t *id = (anna_node_identifier_t *)this;
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
		break;
	    }
	    	    
	    if(!t || t == ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		anna_error(this, L"Unknown identifier: %ls", id->name);
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
	    this = anna_node_specialize(call, stack);	    
	    break;
	}
		
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *call = (anna_node_call_t *)this;
	    
	    /*
	      Do a simple check to see if the specified identifier
	      exists, if it does use regular type calculations. If it
	      doesn't, check aliases.
	    */
	    if(call->function->node_type == ANNA_NODE_IDENTIFIER)
	    {
		anna_node_identifier_t *id = 
		    (anna_node_identifier_t *)call->function;
		anna_type_t *fun_type_simple = 
		    anna_stack_get_type(stack, id->name);
		anna_node_declare_t *fun_decl_simple = 
		    anna_stack_get_declaration(stack, id->name);
		if(!(fun_type_simple || fun_decl_simple))
		{
		    if(anna_node_calculate_type_direct_children(call, stack))
		    {
			wchar_t *unaliased_name = anna_function_search(
			    this->stack, id->name, call);
			
			if(unaliased_name)
			{
			    call->function = (anna_node_t *)anna_node_create_identifier(
				&id->location,
				unaliased_name);
			    call->function->stack = id->stack;
			    call->function = resolve_identifiers_each(
				call->function, 0);
			}
		    }
		}
	    }

	    call->function = anna_node_calculate_type(call->function);
	    
	    anna_type_t *fun_type = call->function->return_type;
	    
	    int is_method = 0;
	    
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
		    assert(type);
		    anna_type_set_stack(type, stack);
		    anna_type_setup_interface(type);
		    
		    this->node_type = ANNA_NODE_CONSTRUCT;
		    call->function = (anna_node_t *)anna_node_create_type(
			&call->function->location,
			type);
		    call->function->stack = call->stack;
		    call->return_type = type;
		    anna_member_t *member = anna_member_get(type, anna_mid_get(L"__init__"));
		    if(!member)
		    {
			anna_type_print(type);
			CRASH;
		    }
		    
		    fun_type = member->type;
		    is_method=1;
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
	    
	    if(anna_node_validate_call_parameters(call, funt, is_method, 1))
	    {
		anna_node_call_map(call, funt, is_method);
	    }
	    
	    if(!is_method)
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
	    call->child[0] = anna_node_calculate_type(call->child[0]);
	    call->child[1] = anna_node_calculate_type(call->child[1]);
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
	case ANNA_NODE_STATIC_MEMBER_CALL:
	{	    
	    this = anna_node_calculate_type_internal_call((anna_node_call_t *)this);
	    break;
	}	
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)this;
	    
	    anna_function_setup_interface(c->payload);
	    if(anna_function_wrap(c->payload))
	    {
		c->return_type = anna_function_wrap(c->payload)->type;
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
	    
	    anna_use_t *use = anna_stack_search_use(c->stack, c->name);
	    if(use)
	    {
		anna_node_member_access_t *this2 = 
		    anna_node_create_member_set(
			0, ANNA_NODE_MEMBER_SET, use->node, anna_mid_get(c->name), c->value);
		this2->stack = c->stack;
		this2->value = anna_node_calculate_type(this2->value);
		this2->return_type = this2->value->return_type;
		this = (anna_node_t *)this2;
	    }
	    else
	    {
		c->value = anna_node_calculate_type(c->value);
		c->return_type = c->value->return_type;
	    }
	    
	    break;
	}

	case ANNA_NODE_NULL:
	{
	    this->return_type = null_type;
	    break;
	}

	case ANNA_NODE_MEMBER_GET:
	case ANNA_NODE_MEMBER_SET:
	case ANNA_NODE_STATIC_MEMBER_GET:
	case ANNA_NODE_STATIC_MEMBER_SET:
	{

	    anna_node_member_access_t *c = (anna_node_member_access_t *)this;

	    int is_static = (this->node_type == ANNA_NODE_STATIC_MEMBER_GET) || 
		(this->node_type == ANNA_NODE_STATIC_MEMBER_SET);
	    
	    

	    c->object = anna_node_calculate_type(c->object);
	    anna_type_t *type = c->object->return_type;
	    if(type == ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		break;
	    }

	    anna_type_set_stack(type, stack);
	    anna_type_setup_interface(type);	    

	    if(is_static)
	    {
		type = anna_node_resolve_to_type(c->object, stack);
		c->access_type = ANNA_NODE_ACCESS_STATIC_MEMBER;
		
		if(!type)
		{
		    anna_error(c->object, L"Unknown type");
		    break;
		}
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

	    if((c->node_type == ANNA_NODE_MEMBER_GET) || 
	       (c->node_type == ANNA_NODE_STATIC_MEMBER_GET))
	    {
		if( (anna_member_is_bound(member)) && 
		    (c->node_type == ANNA_NODE_MEMBER_GET) )

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
		c->value = anna_node_calculate_type(c->value);
		c->return_type = c->value->return_type;		
	    }
	    
	    break;
	}

	case ANNA_NODE_CONST:
	case ANNA_NODE_DECLARE:
	{
	    
	    anna_node_declare_t *d = (anna_node_declare_t *)this;
	    int do_decl = anna_stack_template_search(this->stack, d->name) == this->stack;
//	    debug(D_ERROR, L"Calculating type of declaration %ls\n", d->name);
	    if(d->type->node_type == ANNA_NODE_NULL)
	    {
//		debug(D_ERROR, L"Declaration %ls has implicit type\n", d->name);
		if(d->value->node_type == ANNA_NODE_NULL)
		{
		    anna_error(this, L"No type specified for variable declaration\n");
		}
		if(d->value->node_type == ANNA_NODE_TYPE)
		{
		    d->return_type = type_type;	    
		}
		else
		{
		    d->value = anna_node_calculate_type(d->value);
		    d->return_type = d->value->return_type;
		}
		
		if(d->return_type != ANNA_NODE_TYPE_IN_TRANSIT)
		{
		    if(do_decl)
			anna_stack_set_type(stack, d->name, d->return_type);
		}
	    }
	    else
	    {
//		debug(D_ERROR, L"Declaration %ls has explicit type\n", d->name);
  	        d->type = anna_node_calculate_type(d->type);
		d->return_type = anna_node_resolve_to_type(
		    d->type,
		    stack);
//		debug(D_ERROR, L"Resolved\n");
		if(!d->return_type)
		{
		    anna_error(d->type, L"Invalid type for declaration");
		    d->return_type = ANNA_NODE_TYPE_IN_TRANSIT;
		}
	    }
	    
	    if(this->node_type == ANNA_NODE_CONST)
	    {
		if(do_decl)
		{
		    //debug(D_ERROR, L"Declaration %ls is a constant\n", d->name);
		    anna_entry_t *value = anna_node_static_invoke(
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
	    }
	    
//	    debug(D_ERROR, L"Type calculation of declaration %ls finished\n", d->name);
	    break;
	}
	
	case ANNA_NODE_OR:
	{
	    anna_node_cond_t *d = (anna_node_cond_t *)this;
	    d->arg1 = anna_node_calculate_type(d->arg1);
	    d->arg2 = anna_node_calculate_type(d->arg2);
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
	    d->arg1 = anna_node_calculate_type(d->arg1);
	    d->arg2 = anna_node_calculate_type(d->arg2);
	    d->return_type = d->arg2->return_type;
	    break;
	}
	
	case ANNA_NODE_WHILE:
	{
	    anna_node_cond_t *d = (anna_node_cond_t *)this;
	    d->arg2 = anna_node_calculate_type(d->arg2);
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

	    d->block1 = (anna_node_call_t *)anna_node_calculate_type((anna_node_t *)d->block1);
	    d->block2 = (anna_node_call_t *)anna_node_calculate_type((anna_node_t *)d->block2);
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
	case ANNA_NODE_CONTINUE:
	case ANNA_NODE_BREAK:
	{
	    anna_node_wrapper_t *c = (anna_node_wrapper_t *)this;
	    c->payload = anna_node_calculate_type(c->payload);
	    c->return_type = c->payload->return_type;
	    break;
	}

	case ANNA_NODE_TYPE_OF:
	{	    
	    anna_node_wrapper_t *n = (anna_node_wrapper_t *)this;
	    
	    n->payload = anna_node_calculate_type(n->payload);
	    this->return_type = n->payload->return_type;
	    break;
	}
	
	case ANNA_NODE_RETURN_TYPE_OF:
	{
	    anna_node_wrapper_t *n = (anna_node_wrapper_t *)this;
	    
	    n->payload = anna_node_calculate_type(n->payload);

	    if(n->payload->return_type != ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		anna_function_type_t *fun = anna_function_type_unwrap(
		   n->payload->return_type);
		if(fun)
		{
		    this->return_type = fun->return_type;
		}
	    }
	    break;
	}
	
	case ANNA_NODE_INPUT_TYPE_OF:
	{
	    anna_node_wrapper_t *n = (anna_node_wrapper_t *)this;
	    
	    n->payload = anna_node_calculate_type(n->payload);
	    
	    if(n->payload->return_type != ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		anna_function_type_t *fun = anna_function_type_unwrap(
		   n->payload->return_type);
		if(fun && fun->input_count > n->steps)
		{
		    this->return_type = fun->input_type[n->steps];
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
    return this;
}

static void anna_node_prepare_body(
    anna_node_t *this, void *aux)
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

anna_node_t *anna_node_calculate_type(
    anna_node_t *this)
{

    if(!this->stack)
    {
	anna_error(this,L"Invalid stack value while determining types\n");
	CRASH;
    }
    
//    debug(D_CRITICAL, L"Calculate type of node:\n");
//    anna_node_print(D_CRITICAL, this);
    if(this->transformed && this->transformed != this)
    {
	return anna_node_calculate_type(this->transformed);
    }  
    
    if(this->return_type == ANNA_NODE_TYPE_IN_TRANSIT && anna_error_count == 0)
    {
	anna_error(this, L"Circular type checking dependency");
	this->transformed = this;
    }
    else if(this->return_type == 0)
    {
	
	this->return_type = ANNA_NODE_TYPE_IN_TRANSIT;
	anna_node_t *transformed = anna_node_calculate_type_internal(this);

	if(!this->transformed)
	    this->transformed = transformed;
	
    }
    else
    {
	if(!this->transformed)
	    this->transformed = this;
    }

//    debug(D_SPAM, L"Done\n");
    assert(this->transformed);
    return this->transformed;
}

static anna_node_t *anna_node_calculate_type_fun(
    anna_node_t *this,
    void *aux)
{
    return anna_node_calculate_type(this);
}

void anna_node_calculate_type_children(anna_node_call_t *node)
{
    int i;
    for(i=0; i<node->child_count; i++)
    {
	node->child[i] = anna_node_each_replace(node->child[i], &anna_node_calculate_type_fun, 0);
	if(anna_error_count)
	{
	    return;
	}
    }
    anna_node_each(
	(anna_node_t *)node, 
	&anna_node_prepare_body, 0);
}

static anna_node_t *resolve_identifiers_each(
    anna_node_t *this, void *aux)
{
    if(this->node_type != ANNA_NODE_IDENTIFIER)
    {
	return this;
    }
    
    anna_node_identifier_t *id = (anna_node_identifier_t *)this;
    anna_use_t *use = anna_stack_search_use(
	id->stack,
	id->name);
    
    if(use)
    {
	anna_node_t *src_node = anna_node_clone_deep(use->node);
	src_node->stack = this->stack;
	src_node = anna_node_calculate_type(src_node);
	anna_node_t *res = (anna_node_t *)anna_node_create_member_get(
	    &id->location,
	    ANNA_NODE_MEMBER_GET,
	    src_node,
	    anna_mid_get(id->name));
	anna_node_set_stack(res, id->stack);
	return res;
    }
    return this;    
}

void anna_node_resolve_identifiers(
    anna_node_t *this)
{
    anna_node_each_replace(
	this, resolve_identifiers_each, 0);
}
