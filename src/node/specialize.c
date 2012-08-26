
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
	anna_entry_t val = anna_node_static_invoke_try(call->function, call->function->stack);
	anna_function_t *fun;
	if(!anna_entry_null_ptr(val) && (fun=anna_function_unwrap(anna_as_obj(val))))
	{
	    spec_fun = anna_function_get_specialization(fun, call);
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

void *anna_specialize_implicit(
    anna_node_call_t *attr, anna_function_type_t *unspecialized_fun,
    anna_node_call_t *input_node, anna_node_call_t *call, void *base,
    anna_specializer_t specializer)
{
    int i;
    array_list_t al = AL_STATIC;
    anna_attribute_call_all(attr, L"template", &al);

    if(al_get_count(&al) == 0)    
    {
	return base;
    }

    int input_count = unspecialized_fun->input_count;

    if(!anna_node_validate_call_parameters(call, unspecialized_fun, 0))
    {
	return base;
    }
    
    anna_node_call_map(call, unspecialized_fun);
    
    anna_type_t **type_spec = calloc(sizeof(anna_type_t *), al_get_count(&al));
    int spec_count = 0;
    for(i=0; i<call->child_count; i++)
    {	
	int input_idx = mini(i, input_count-1);	
	anna_node_call_t *decl = node_cast_call(input_node->child[input_idx]);
	//anna_message(L"Hej hopp %d %d\n", i, input_idx);
	if(decl->child[1]->node_type == ANNA_NODE_INTERNAL_IDENTIFIER)
	{
	    anna_node_identifier_t *id =(anna_node_identifier_t *)decl->child[1];
	    //anna_message(L"Tjoho %ls\n", id->name);
	    
	    int templ_idx = anna_attribute_template_idx(attr, id->name);
	    if(templ_idx >= 0)
	    {
		//anna_message(L"Template idx is %d\n", templ_idx);
		call->child[i] = anna_node_calculate_type(call->child[i]);
		if( call->child[i]->return_type != ANNA_NODE_TYPE_IN_TRANSIT)
		{
		    if(!type_spec[templ_idx])
		    {
			type_spec[templ_idx] = call->child[i]->return_type;
			spec_count++;
		    }
		    else
		    {
			type_spec[templ_idx] = anna_type_intersect(type_spec[templ_idx], call->child[i]->return_type);
		    }
		}
	    }
	}
    }
    
    if(spec_count == al_get_count(&al))
    {
	anna_node_call_t *spec_call = anna_node_create_block2(0);
	for(i=0; i<al_get_count(&al); i++)
	{
	    anna_node_call_push(
		spec_call, 
		(anna_node_t *)anna_node_create_dummy(
		    0,
		    anna_type_wrap(type_spec[i])));
	}
	base = specializer(base, spec_call);
    }

    al_destroy(&al);
    free(type_spec);
    return base;
}

