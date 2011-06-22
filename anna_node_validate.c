static int anna_node_f_get_index(anna_function_type_t *f, int is_method, wchar_t *name)
{
    int i;
    for(i=(!!is_method); i<f->input_count; i++)
    {
	if(wcscmp(name, f->input_name[i]) == 0)
	{
	    return i - !!is_method;
	}
    }
    return -1;
}

static void anna_node_validate_call(anna_node_t *this, anna_stack_template_t *stack)
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
	    return;
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
	
	anna_type_t * type = 
	    this2->object->return_type;
				
	if(this2->access_type == ANNA_NODE_ACCESS_STATIC_MEMBER)
	{
	    type = anna_node_resolve_to_type(this2->object, stack);
	}
	    
	anna_member_t *memb = anna_member_get(type, this2->mid);
	anna_type_t *ft = memb->type;
		
	ftk = anna_function_type_unwrap(ft);	    
		
	if(ftk)
	{
	    tmpl = ftk->input_type;
	    tmpl_count = ftk->input_count;
	    if(memb->is_bound_method)
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
	    return;
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
	return;
    }
    if(ftk->flags & ANNA_FUNCTION_VARIADIC)
    {
	if( this2->child_count < tmpl_count-1)
	{
	    anna_error(
		this,
		L"Too few parameters to function call. Expected at least %d, got %d\n", 
		tmpl_count-1, this2->child_count);
	    return;
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
	    return;
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
}


void anna_node_validate(anna_node_t *this, anna_stack_template_t *stack)
{
    switch(this->node_type)
    {
	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_access_t *c = (anna_node_member_access_t *)this;
	    anna_type_t * type = 
		c->object->return_type;
	    if(c->access_type == ANNA_NODE_ACCESS_STATIC_MEMBER)
	    {
		type = anna_node_resolve_to_type(c->object, stack);
	    }
	    anna_member_t *memb = anna_member_get(type, c->mid);
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
	    anna_type_t * type = 
		c->object->return_type;
	    if(c->access_type == ANNA_NODE_ACCESS_STATIC_MEMBER)
	    {
		type = anna_node_resolve_to_type(c->object, stack);
	    }

	    anna_member_t *memb = anna_member_get(type, c->mid);
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
	    anna_node_validate_call(this, stack);
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

int anna_node_validate_call_parameters(
    anna_node_call_t *call, 
    anna_function_type_t *target, 
    int is_method, 
    int print_error)
{
    anna_type_t **param = target->input_type;
    wchar_t **param_name = target->input_name;
    int param_count = target->input_count;    
    int res=0;
    
    if(anna_function_type_is_variadic(target))
	return 1;
    
    if(is_method)
    {
	param++;
	param_name++;
	param_count--;
    }
    
    int i;
    int *set = calloc(sizeof(int), param_count);
    int has_named=0;

    if(param_count < call->child_count)
    {
	if(print_error)
	{
	    anna_error((anna_node_t *)call, L"Wrong number of parameters to function call. Got %d, expected %d.", call->child_count, param_count);
	}
	goto END;
    }
    
    for(i=0; i<call->child_count; i++)
    {
	
	int is_named = call->child[i]->node_type == ANNA_NODE_MAPPING;
	if(has_named && !is_named)
	{
	    if(print_error)
	    {
		anna_error(call->child[i], L"An anonymous parameter value can not follow after a named parameter value");
	    }
	    goto END;
	}
	int idx = i;
	if(is_named)
	{
	    anna_node_cond_t *p = (anna_node_cond_t *)call->child[i];
	    if(p->arg1->node_type != ANNA_NODE_INTERNAL_IDENTIFIER)
	    {
		if(print_error)
		{
		    anna_error(call->child[i], L"Invalid named parameter %d", p->arg1->node_type);
		}
		goto END;
	    }
	    anna_node_identifier_t *name = (anna_node_identifier_t *)p->arg1;	    
	    idx = anna_node_f_get_index(target, is_method, name->name);
	    if(idx < 0)
	    {
		if(print_error)
		{
		    anna_error(call->child[i], L"Invalid named parameter");
		}
		goto END;
	    }
	}
	set[idx]++;
    }
    for(i=0; i<param_count; i++)
    {
	if(set[i] == 0 && target->input_default[i])
	{
	    set[i]++;
	}
    }
    
    for(i=0; i<param_count; i++)
    {
	if(set[i] > 1)
	{
	    if(print_error)
	    {
		anna_error((anna_node_t *)call, L"More than one value was provided for argument %d, %ls, in function call ", i+1, param_name[i]);
	    }
	    goto END;
	}
	else if(set[i] < 1)
	{
	    if(print_error)
	    {
		anna_error((anna_node_t *)call, L"No value was provided for argument %d, %ls, in function call ", i+1, param_name[i]);
	    }
	    goto END;	    
	}
	
    }
    res = 1;

  END:
    free(set);
    return res;
}

void anna_node_call_map(
    anna_node_call_t *call, 
    anna_function_type_t *target, 
    int is_method)
{
    if(anna_function_type_is_variadic(target))
	return;

    anna_type_t **param = target->input_type;
    int param_count = target->input_count;    
    
    if(is_method)
    {
	param++;
	param_count--;
    }
    
    int i;
    size_t order_sz = sizeof(anna_node_t *)* param_count;
    anna_node_t **order = calloc(1, order_sz);

    for(i=0; i<call->child_count; i++)
    {
	int is_named = call->child[i]->node_type == ANNA_NODE_MAPPING;
	if(is_named)
	{
	    anna_node_cond_t *p = (anna_node_cond_t *)call->child[i];
	    anna_node_identifier_t *name = (anna_node_identifier_t *)p->arg1;	    
	    int idx = anna_node_f_get_index(target, is_method, name->name);
	    order[idx] = p->arg2;
	}
	else
	{
	    order[i] = call->child[i];
	}
    }
    for(i=0; i<param_count; i++)
    {
	if(!order[i])
	{
	    order[i] = target->input_default[i];
	}
    }
    if(call->child_count != param_count)
    {
	call->child_count = param_count;	
	call->child = realloc(call->child, order_sz);
	
    }
    memcpy(call->child, order, order_sz);
    

    free(order);
    return;
    
}

