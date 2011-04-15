
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


static void anna_node_calculate_type_param(
    size_t argc,
    anna_node_t **argv,
    int is_method,
    anna_function_type_t *funt)
{
    int i, j;
    for(i=0; i<argc; i++)
    {
	if(argv[i]->node_type == ANNA_NODE_CLOSURE)
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)argv[i];
	    anna_function_t *closure = c->payload;
//	    debug(D_SPAM,L"Closure as param %d. function type says argument is of type %ls\n", i, funt->input_type[i+!!is_method]->name);
	    anna_function_type_t *template = anna_function_type_extract(funt->input_type[i+!!is_method]);
	    assert(template);
//	    debug(D_CRITICAL,L"Closure template takes %d params\n", template->input_count);
	    for(j=0; j<template->input_count; j++)
	    {
//		debug(D_CRITICAL,L"Argument %d should be of type %ls\n", j, template->input_type[j]->name);
		anna_function_argument_hint(
		    closure,
		    j,
		    template->input_type[j]);
	    }
	}
    }
}

void anna_node_register_declarations(
    anna_stack_template_t *stack,
    anna_node_t *this)
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

static void anna_node_calculate_type_internal(
    anna_node_t *this,
    anna_stack_template_t *stack)
{
    
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
	    anna_node_specialize(call, stack);
	    
	    break;
	}
		
	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *call = (anna_node_call_t *)this;
	    anna_node_calculate_type(call->function, stack);
	    anna_type_t *fun_type = call->function->return_type;

	    if(fun_type == type_type)
	    {
//		debug(D_SPAM,L"Hmmm, node is of type type...");
//		anna_node_print(0, call->function);
		
		anna_type_t *type = anna_node_resolve_to_type(call->function, stack);
		if(type)
		{
		    this->node_type = ANNA_NODE_CONSTRUCT;
		    call->function = (anna_node_t *)anna_node_create_type(
			&call->function->location,
			type);
		    call->return_type = type;
		    break;
		}
		
	    }

	    if(fun_type == ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		break;
	    }

	    anna_function_type_t *funt = anna_function_type_extract(fun_type);
	    if(!funt)
	    {
		anna_error(this, L"Value is not callable");
		break;
	    }
	    
	    if(funt->flags & ANNA_FUNCTION_MACRO)
	    {
		anna_error(this, L"Found unexpanded macro call while calculating function return type");
		
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
	    anna_node_calculate_type(call->child[1], stack);
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
	    
	    anna_node_calculate_type(n->object, stack);
	    anna_type_t *type = n->object->return_type;
	    if(type == ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		break;
	    }
	    
	    anna_type_prepare_member(type, n->mid, stack);
	    anna_member_t *member = anna_member_get(type, n->mid);
	    
	    if(!member)
	    {

		int i;
		int ok = 1;
		
		for(i=0; i<n->child_count; i++)
		{
		    anna_node_calculate_type(n->child[i], stack);
		    if(n->child[i]->return_type == ANNA_NODE_TYPE_IN_TRANSIT)
		    {
			ok = 0;
			break;
		    }
		}
		
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
		anna_function_type_t *fun = anna_function_unwrap_type(member->type);
		
		if(!anna_node_call_validate(n, fun, 1, 1))
		{
		    member = 0;
		}
		else
		{
		    anna_node_call_map(n, fun, 1);
		    
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

	    anna_function_type_t *funt = anna_function_type_extract(member->type);
	    n->return_type = funt->return_type;
	    
	    anna_node_calculate_type_param(n->child_count, n->child, 1, funt);
	    
	    break;
	}
	
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)this;
	    
	    anna_function_setup_interface(c->payload, stack);
	    if(c->payload->wrapper)
	    {
		c->return_type = c->payload->wrapper->type;
	    }
	    
	    break;
	}
	
	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *c = (anna_node_type_t *)this;
	    anna_type_t *f = c->payload;
	    c->return_type = type_type;
	    
	    if(f->definition)
	    {
		anna_type_setup_interface(f, stack);
	    }
	    break;
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
	    anna_node_member_access_t *c = (anna_node_member_access_t *)this;
	    anna_node_calculate_type(c->object, stack);
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
	    
	    if(member->is_method)
	    {
		c->node_type = ANNA_NODE_MEMBER_GET_WRAP;
		c->return_type = anna_method_curry(anna_function_unwrap_type(member->type));
	    }
	    else
	    {
		c->return_type = member->type;
	    }
	    
	    break;
	}

	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_access_t *g = (anna_node_member_access_t *)this;
	    anna_node_calculate_type(g->value, stack);
	    g->return_type = g->value->return_type;
	    break;
	}
	
	case ANNA_NODE_CONST:
	case ANNA_NODE_DECLARE:
	{
	    anna_node_declare_t *d = (anna_node_declare_t *)this;
//	    debug(D_ERROR, L"Calculating type of declaration %ls\n", d->name);
	    if(d->type->node_type == ANNA_NODE_IDENTIFIER)
	    {
//		debug(D_ERROR, L"Declaration %ls has identifier as type\n", d->name);
		anna_node_identifier_t *t = node_cast_identifier(d->type);
		anna_object_t *t2 = anna_stack_get(stack, t->name);	    
		d->return_type = anna_type_unwrap(t2);
	    }
	    else if(d->type->node_type == ANNA_NODE_NULL)
	    {
//		debug(D_ERROR, L"Declaration %ls has implicit type\n", d->name);
		if(d->value->node_type == ANNA_NODE_NULL)
		{
		    anna_error(this, L"No type specified for variable declaration\n");
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
	    
	    if(this->node_type == ANNA_NODE_CONST)
	    {
		anna_object_t *value = anna_node_static_invoke(
		    d->value, stack);
		anna_stack_set(
		    stack,
		    d->name,
		    value);
	    }
	    
//	    debug(D_ERROR, L"Type calculation of declaration %ls finished\n", d->name);
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
	case ANNA_NODE_MAPPING:
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
	    anna_type_t *fun_type =  d->arg2->return_type;
	    if(fun_type == ANNA_NODE_TYPE_IN_TRANSIT)
	    {
		break;
	    }
	    anna_function_type_t *funt = anna_function_type_extract(fun_type);
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
	
	case ANNA_NODE_RETURN:
	{
	    anna_node_wrapper_t *c = (anna_node_wrapper_t *)this;
	    anna_node_calculate_type(c->payload, stack);
	    c->return_type = c->payload->return_type;
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
    anna_stack_template_t *stack)
{
    debug(D_SPAM, L"Calculate type of node:\n");
    anna_node_print(D_SPAM, this);
    if(this->return_type == ANNA_NODE_TYPE_IN_TRANSIT)
    {
	anna_error(this, L"Circular type checking dependency");
	anna_node_print(0, this);
    }
    else if(this->return_type == 0)
    {
	this->return_type = ANNA_NODE_TYPE_IN_TRANSIT;
	anna_node_calculate_type_internal( this, stack );
    }
    debug(D_SPAM, L"Done\n");
}

void anna_node_validate(anna_node_t *this, anna_stack_template_t *stack)
{
    switch(this->node_type)
    {
	case ANNA_NODE_MEMBER_CALL:
	case ANNA_NODE_CONSTRUCT:
	{
	    anna_function_type_t *ftk=0;
	    int child_count;
	    anna_node_t **child;

	    if(this->node_type == ANNA_NODE_CONSTRUCT)
	    {
		anna_node_call_t *this2 =(anna_node_call_t *)this;
		anna_type_t *ft = this2->function->return_type;
		if(!ft)
		{
		    anna_error(this, L"Invalid return type");
		    break;
		}
	    
		anna_node_type_t *tn = (anna_node_type_t *)this2->function;
		
		anna_object_t **constructor_ptr = anna_static_member_addr_get_mid(
		    tn->payload,
		    ANNA_MID_INIT_PAYLOAD);
		assert(constructor_ptr);
		ftk = anna_function_type_extract(
		    (*constructor_ptr)->type);
		child = this2->child;
		child_count = this2->child_count;
	    }
	    else
	    {
		anna_node_call_t *this2 =(anna_node_call_t *)this;
		anna_type_t *ft = 
		    anna_member_get(this2->object->return_type, this2->mid)->type;
		
		ftk = anna_function_type_extract(ft);	    
		child = this2->child;
		child_count = this2->child_count;		
	    }
	    
	    if(!ftk)
	    {
		anna_error(this, L"Tried to call a non-function");
		break;
	    }
	    if(ftk->flags & ANNA_FUNCTION_VARIADIC)
	    {
		if( child_count < ftk->input_count-2)
		{
		    anna_error(
			this,
			L"Too few parameters to constructor call. Expected at least %d, got %d\n", 
			ftk->input_count-2, child_count);
		    break;
		}
		
	    }
	    else
	    {
		if(ftk->input_count != child_count+1)
		{
		    anna_error(
			this,
			L"Wrong number of parameters to constructor call. Expected %d, got %d\n", 
			ftk->input_count-1, child_count);
		    anna_node_print(D_ERROR, this);
		    break;
		}
	    }
	    int i;
	    for(i=0; i<child_count; i++)
	    {
		anna_type_t *param = child[i]->return_type;
		anna_type_t *templ = ftk->input_type[mini(i+1, ftk->input_count-1)];
		if(!anna_abides(param, templ))
		{
		    
		    anna_error(
			this,
			L"Invalid type of parameter %d in function call. Expected type %ls, got type %ls", i+1, templ->name, param->name);
		}
		
	    }
	    	    
	    break;	    
	}

	case ANNA_NODE_CALL:
	{
	    anna_node_call_t *this2 =(anna_node_call_t *)this;
	    anna_type_t *ft = this2->function->return_type;
	    if(!ft)
	    {
		anna_error(this, L"Invalid return type");
		break;
	    }
	    
	    anna_function_type_t *ftk;
	    ftk = anna_function_type_extract(ft);
	    
	    if(!ftk)
	    {
		anna_error(this, L"Tried to call a non-function");
		break;
	    }
	    if(ftk->flags & ANNA_FUNCTION_VARIADIC)
	    {
		if( this2->child_count < ftk->input_count-1)
		{
		    anna_error(
			this,
			L"Too few parameters to function call. Expected at least %d, got %d\n", 
			ftk->input_count-1, this2->child_count);
		    break;
		}
		
	    }
	    else
	    {
		if(ftk->input_count != this2->child_count)
		{
		    anna_error(
			this,
			L"Wrong number of parameters to function call. Expected %d, got %d\n", 
			ftk->input_count, this2->child_count);
		    break;
		}
	    }
	    int i;
	    for(i=0; i<this2->child_count; i++)
	    {
		anna_type_t *param = this2->child[i]->return_type;
		anna_type_t *templ = ftk->input_type[mini(i, ftk->input_count-1)];
//		wprintf(L"Check if type %ls abides to %ls\n", this2->child[i]->return_type->name, ftk->input_type[mini(i, ftk->input_count-1)]->name);
		if(!anna_abides(param, templ))
		{
		    
		    anna_error(
			this,
			L"Invalid type of parameter %d in function call. Expected type %ls, got type %ls", i+1, templ->name, param->name);
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
		    L"Invalid type in assignment. Expected type %ls, got type %ls", 
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

void anna_node_calculate_type_children(anna_node_call_t *node, anna_stack_template_t *stack)
{
    int i;
    for(i=0; i<node->child_count; i++)
    {
	anna_node_each(node->child[i], (anna_node_function_t)&anna_node_calculate_type, stack);
	if(anna_error_count)
	{
	    return;
	}
    }
    anna_node_each((anna_node_t *)node, (anna_node_function_t)&anna_node_prepare_body,stack);
}
