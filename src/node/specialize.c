
static anna_node_t *anna_node_specialize(anna_node_call_t *call, anna_stack_template_t *stack)
{
    anna_node_calculate_type(call->function);
    
//    wprintf(L"Weee, specializing\n");
//    anna_node_print(4, call);
        
    anna_type_t *type = anna_node_resolve_to_type(call->function, stack);
    anna_type_t *res = 0;

    if(!type)
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
	res = hash_get(&type->specializations, call);
	if(!res)
	{
	    res = anna_type_specialize(type, call);
	    
	    if(!res)
	    {
		anna_error((anna_node_t *)call, L"Failed to specialize type %ls.", type->name);
	    }
	}
    }
    
    if(res)
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
