
static anna_node_t *anna_node_specialize(anna_node_call_t *call, anna_stack_template_t *stack)
{
    anna_node_calculate_type(call->function);
    
    anna_type_t *type = anna_node_resolve_to_type(call->function, stack);
    anna_type_t *res = 0;
    anna_function_t *spec_fun = 0;
	    
    	    
    if(!type || type == ANNA_NODE_TYPE_IN_TRANSIT)
    {
	anna_error((anna_node_t *)call, L"Invalid template type");
    }
    else if(type == mutable_list_type && call->child_count==1)
    {
	call->child[0] = anna_node_calculate_type(call->child[0]);	
	anna_type_t *spec = anna_node_resolve_to_type(call->child[0], stack);

	if(spec)
	{
	    res = anna_list_type_get_mutable(spec);
	}
	else
	{
	    anna_error((anna_node_t *)call, L"List specialization can not be resolved into type");
	}
    }
    else if(type == any_list_type && call->child_count==1)
    {
	call->child[0] = anna_node_calculate_type(call->child[0]);	
	anna_type_t *spec = anna_node_resolve_to_type(call->child[0], stack);
	
	if(spec)
	{
	    res = anna_list_type_get_any(spec);
	}
	else
	{
	    anna_error((anna_node_t *)call, L"List specialization can not be resolved into type");
	}
    }
    else if(type == imutable_list_type && call->child_count==1)
    {
	call->child[0] = anna_node_calculate_type(call->child[0]);	
	anna_type_t *spec = anna_node_resolve_to_type(call->child[0], stack);
	if(spec)
	{
	    res = anna_list_type_get_imutable(spec);
	}
	else
	{
	    anna_error((anna_node_t *)call, L"List specialization can not be resolved into type");
	}
    }
    else if(type == hash_type && call->child_count==2)
    {
	call->child[0] = anna_node_calculate_type(call->child[0]);	
	call->child[1] = anna_node_calculate_type(call->child[1]);	
	anna_type_t *spec1 = anna_node_resolve_to_type(call->child[0], stack);
	anna_type_t *spec2 = anna_node_resolve_to_type(call->child[1], stack);
	if(spec1 && spec2)
	{
	    res = anna_hash_type_get(spec1, spec2);
	}
	else
	{
	    anna_error((anna_node_t *)call, L"HashMap specializations can not be resolved into types");
	}
    }
    else if(type == pair_type && call->child_count==2)
    {
	call->child[0] = anna_node_calculate_type(call->child[0]);	
	call->child[1] = anna_node_calculate_type(call->child[1]);	
	anna_type_t *spec1 = anna_node_resolve_to_type(call->child[0], stack);
	anna_type_t *spec2 = anna_node_resolve_to_type(call->child[1], stack);

	if(spec1 && spec2)
	{
	    res = anna_pair_type_get(spec1, spec2);
	}
	else
	{
	    anna_error((anna_node_t *)call, L"Pair specializations can not be resolved into types");
	}
    }
    else
    {	
	anna_entry_t *val = anna_node_static_invoke_try(call->function, call->function->stack);
	anna_function_t *fun;	
	if(val && (fun=anna_function_unwrap(anna_as_obj(val))))
	{
	    anna_message(L"LALALA\n");
	    anna_node_print(99, (anna_node_t *)call);
	    spec_fun = hash_get(&fun->specialization, call);
	    
	    if(!spec_fun)
	    {
		spec_fun = anna_function_create_specialization(fun, call);
		if(spec_fun)
		{
		    hash_put(&fun->specialization, call, spec_fun);
		}
	    }
	    
	}
	else
	{
	    res = hash_get(&type->specialization, call);
	    if(!res)
	    {
		res = anna_type_specialize(type, call);
		if(res)
		{
		    hash_put(&type->specialization, call, res);
		}
	    }
	    if(!res)
	    {
		anna_error((anna_node_t *)call, L"Failed to specialize type %ls.", type->name);
	    }
	}
    }
    
    if(spec_fun)
    {
	res = anna_function_wrap(spec_fun)->type;
	anna_node_t *out = (anna_node_t *)anna_node_create_closure(&call->location, spec_fun);
	out->return_type = res;
	out->stack = call->stack;
	return out;
    }	    
    else if(res)
    {
        anna_node_dummy_t *out = anna_node_create_dummy(
	    &call->location,
	    anna_type_wrap(res));
	out->return_type = type_type;
	out->stack = call->stack;
	return (anna_node_t *)out;
    }
    
    return (anna_node_t *)call;	    
}
