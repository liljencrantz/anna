
static void anna_node_specialize(anna_node_call_t *call, anna_stack_template_t *stack)
{
    anna_node_calculate_type(call->function, stack);
    
    anna_type_t *type = anna_node_resolve_to_type(call->function, stack);


    if(!type)
    {
	anna_error((anna_node_t *)call, L"Invalid template type");
    }
    else if(type == list_type && call->child_count==1)
    {
	anna_type_t *spec = anna_node_resolve_to_type(call->child[0], stack);
	if(spec)
	{
	    anna_type_t *res = anna_list_type_get(spec);
	    
	    /* FIXME: We remake this node into a new one of a different type- Very, very fugly. Do something prettier, please? */
	    anna_node_dummy_t *new_res = (anna_node_dummy_t *)call;
	    new_res->node_type = ANNA_NODE_DUMMY;
	    new_res->payload = anna_type_wrap(res);
	    new_res->return_type = type_type;
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
	    anna_type_t *res = anna_hash_type_get(spec1, spec2);
		    
	    /* FIXME: We remake this node into a new one of a different type- Very, very fugly. Do something prettier, please? */
	    anna_node_dummy_t *new_res = (anna_node_dummy_t *)call;
	    new_res->node_type = ANNA_NODE_DUMMY;
	    new_res->payload = anna_type_wrap(res);
	    new_res->return_type = type_type;
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
	    anna_type_t *res = anna_pair_type_get(spec1, spec2);
		    
	    /* FIXME: We remake this node into a new one of a different type- Very, very fugly. Do something prettier, please? */
	    anna_node_dummy_t *new_res = (anna_node_dummy_t *)call;
	    new_res->node_type = ANNA_NODE_DUMMY;
	    new_res->payload = anna_type_wrap(res);
	    new_res->return_type = type_type;
	}
	else
	{
	    anna_error((anna_node_t *)call, L"Pair specializations can not be resolved into types");
	}
		
    }
    else
    {
	anna_type_t *res = hash_get(&type->specializations, call);
	    
	anna_error((anna_node_t *)call, L"Unimplemented template specialization. Come back tomorrow.");
    }
	    
}
