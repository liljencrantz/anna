//ROOT: src/type.c

#define ABIDES_IN_TRANSIT -1

static hash_table_t anna_abides_cache;
static hash_table_t anna_intersect_cache;

void anna_abides_init()
{
    hash_init(&anna_abides_cache, hash_tt_func, hash_tt_cmp);
    hash_init(&anna_intersect_cache, hash_tt_func, hash_tt_cmp);
}

static int anna_abides_function(
    anna_function_type_t *contender,
    anna_function_type_t *role_model,
    int is_method,
    int check_type, 
    int verbose)
{
    int i;

    if(contender->input_count != role_model->input_count)
    {
	debug(
	    verbose, L"Input count mismatch, should be %d, is %d.\n", 
	    role_model->input_count, contender->input_count);
	return 0;
    }
    
    if(check_type)
    {
	for(i=!!is_method; i<contender->input_count; i++)
	{
	    if(!anna_abides(contender->input_type[i], role_model->input_type[i]))
	    {
		debug(
		    verbose, 
		    L"Input %d mismatches, %ls does not abide to %ls\n", 
		    i, contender->input_type[i]->name,
		    role_model->input_type[i]->name );
		return 0;
	    }
	}
    }

    if(check_type)
    {
	if(!anna_abides(contender->return_type, role_model->return_type))
	{
	    debug(
		verbose,
		L"Return type mismatch. %ls does not abide by %ls\n",
		contender->return_type->name, role_model->return_type->name);
	    return 0;
	}
    }
    
    return 1;
}

static int anna_abides_fault_count_internal(
    anna_type_t *contender, 
    anna_type_t *role_model, 
    int verbose)
{
    
    if(contender == role_model || contender == null_type)
    {
	return 0;
    }

    anna_tt_t tt = 
	{
	    contender, role_model
	};    
    
    long count = (long)hash_get(&anna_abides_cache, &tt);
    if(count == ABIDES_IN_TRANSIT)
    {
	return 0;
    }
    else if(count != 0)
    {
	debug(verbose, L"Cached result is %d\n", count-1);
	return count - 1;
    }
/*    
    if(role_model == range_type)
    {
	verbose += 10;
	anna_message(L"WEE %ls\n", contender->name);
    }
*/  

    anna_tt_t *key = anna_tt_make(contender, role_model);
    
    hash_put(&anna_abides_cache, key, (void *)(long)ABIDES_IN_TRANSIT);
    
    size_t i;
    int res = 0;    

    anna_function_type_t *c_fun_type = anna_function_type_unwrap(contender);
    anna_function_type_t *r_fun_type = anna_function_type_unwrap(role_model);
    
    if(r_fun_type)
    {
	if(c_fun_type)
	{
	    res += !anna_abides_function(c_fun_type, r_fun_type, 0, 1, verbose);
	}
	else
	{
	    res++;
	    debug(verbose, L"Template is callable.\n");
	}
    }

    if(anna_type_sendable(role_model) && !anna_type_sendable(contender))
    {
	debug(
	    verbose,
	    L"Contender is not sendable\n",
	    res);
	res++;
    }

    
    for(i=0; i<anna_type_get_member_count(role_model); i++)
    {
	anna_member_t *r_memb = anna_type_get_member_idx(
	    role_model, 
	    i);
	if(wcscmp(r_memb->name, L"__init__") == 0)
	{
	    continue;
	}
	
	anna_member_t *c_memb = anna_member_get(
	    contender, 
	    anna_mid_get(r_memb->name));	
	int ok=1;
	if(!c_memb)
	{
	    ok=0;
	    debug(verbose, L"No member named %ls\n", r_memb->name);
	}
	else if(anna_member_is_bound(r_memb) != anna_member_is_bound(c_memb))
	{
	    ok=0;
	    debug(
		verbose,
		L"Miss on %ls because of one is a method and not the other\n", 
		r_memb->name);

	}
	else if(anna_member_is_static(r_memb) != anna_member_is_static(c_memb))
	{
	    debug(
		verbose,
		L"Miss on %ls because one is static and not the other\n",
		r_memb->name);
	    ok=0;
	}
	else if(anna_member_is_bound(r_memb))
	{
	    ok = anna_abides_function(
		anna_function_type_unwrap(c_memb->type),
		anna_function_type_unwrap(r_memb->type),
		1, 1, verbose);
	    if(!ok)
	    {
		debug(verbose, L"Miss on %ls because of method signature mismatch\n", r_memb->name);
	    }
	}
	else
	{
	    ok = anna_abides(c_memb->type, r_memb->type);
	    if(!ok)
	    {
		debug(
		    verbose, L"Miss on %ls because of %ls\n",
		    r_memb->name, 
		    c_memb->type?L"incompatibility":L"missing member");
	    }
	}
	res += !ok;
    }
    
    debug(
	verbose,
	L"Found %d errors\n",
	res);
    

    hash_put(&anna_abides_cache, key, (void *)(long)(res+1));
    
    return res;
}

int anna_abides_fault_count(anna_type_t *contender, anna_type_t *role_model)
{
    return anna_abides_fault_count_internal(contender, role_model, 0);
}

int anna_abides(anna_type_t *contender, anna_type_t *role_model)
{
    return !anna_abides_fault_count(contender, role_model);
}

void anna_type_intersect_into(
    anna_type_t *res, anna_type_t *t1, anna_type_t *t2)
{
    int i;
    anna_tt_t *tt = malloc(sizeof(anna_tt_t));
    
    if(t1 < t2)
    {
	tt->type1 = t1;
	tt->type2 = t2;
    }
    else
    {
	tt->type1 = t2;
	tt->type2 = t1;	
    }    
    
    hash_put(&anna_intersect_cache, tt, res);

    anna_function_type_t *ft1 = anna_function_type_unwrap(t1);
    anna_function_type_t *ft2 = anna_function_type_unwrap(t2);
    if(ft1 && ft2)
    {
	FIXME("Intersections of callable types don't intersect the call");	
    }
    anna_type_t *init_argv[] = 
	{
	    res
	};
    wchar_t *init_argn[] = 
	{
	    L"this"
	};

    anna_member_create_native_method(
	res,
	ANNA_MID_INIT,
	0,
	&anna_vm_null_function,
	res,
	1,
	init_argv,
	init_argn, 0, 
	0);
    
    
    for(i=0; i<anna_type_get_member_count(t2); i++)
    {
	anna_member_t *memb2 = anna_type_get_member_idx(t2, i);
	int mid = anna_mid_get(memb2->name);
	if(wcscmp(memb2->name, L"__init__") == 0)
	    continue;

	anna_member_t *memb1 = anna_member_get(
	    t1,
	    mid);
	if(!memb1)
	{
//	    anna_message(L"Skip %ls\n", members[i]);
	    continue;
	}
	else if(anna_member_is_bound(memb1) != anna_member_is_bound(memb2))
	{
//	    anna_message(L"Skip %ls\n", members[i]);
	    continue;
	}
	else if(anna_member_is_static(memb1) != anna_member_is_static(memb2))
	{
//	    anna_message(L"Skip %ls\n", members[i]);
	    continue;
	}
	else if(anna_member_is_bound(memb2))
	{
	    anna_function_type_t *ft1 = anna_function_type_unwrap(memb1->type);
	    anna_function_type_t *ft2 = anna_function_type_unwrap(memb2->type);
	    
	    if(
		anna_abides_function(
		    ft1, ft2, 1, 0, 0)) 
	    {
		anna_type_t **types = 
		    malloc(sizeof(anna_type_t *)*ft2->input_count);
		anna_node_t **defaults = 
		    calloc(1,sizeof(anna_node_t *)*ft2->input_count);
		int i;
		for(i=0; i<ft2->input_count; i++)
		{
		    types[i] = anna_type_intersect(ft1->input_type[i], ft2->input_type[i]);		    
		    if(ft1->input_default[i] && ft2->input_default[i] && 
		       anna_node_compare(
			   ft1->input_default[i],
			   ft2->input_default[i])==0)
		    {
			defaults[i] = ft1->input_default[i];
		    }
		}
		
		anna_member_create_native_method(
		    res,
		    anna_mid_get(memb2->name),
		    ft2->flags,
		    &anna_vm_null_function,
		    anna_type_intersect(ft1->return_type,ft2->return_type),
		    ft2->input_count,
		    types,
		    ft2->input_name,
		    defaults,
		    0);
		free(types);
		free(defaults);
		
/*		anna_message(
		    L"FDASFDSA %ls.%ls %d\n",
		    t1->name, memb2->name,
		    anna_entry_get_static(
			t1, 
			anna_mid_get(
			    memb2->name)));
*/		
		array_list_t alias = AL_STATIC;
		anna_attribute_call_all(memb1->attribute, L"alias", &alias);
		
		for(i=0; i<al_get_count(&alias); i++)
		{
		    anna_node_t *al = al_get(&alias, i);
		    
		    if(al->node_type == ANNA_NODE_IDENTIFIER)
		    {
			anna_node_identifier_t *nam = (anna_node_identifier_t *)al;
			if(anna_attribute_has_alias(memb2->attribute, nam->name))
			{
			    anna_member_alias(res, mid, nam->name);
			}
		    }		    
		}
		al_destroy(&alias);
		
	    }
	}
	else
	{
	    anna_type_t *member_type = 0;
	    
	    if(anna_abides(memb1->type, memb2->type) && anna_abides(memb2->type, memb1->type))
	    {
		member_type = memb2->type;
	    }
	    else
	    {
		member_type = anna_type_intersect(memb1->type, memb2->type);
	    }
	    
	    int getter = 1;
	    int setter = 1;
		
	    if (memb1->storage & ANNA_MEMBER_PROPERTY)
	    {
		if(memb1->getter_offset == -1)
		    getter = 0;
		if(memb1->setter_offset == -1)
		    setter = 0;
	    }
	    
	    if (memb2->storage & ANNA_MEMBER_PROPERTY)
	    {
		if(memb2->getter_offset == -1)
		    getter = 0;
		if(memb2->setter_offset == -1)
		    setter = 0;
	    }
	    
	    if (getter && setter)
	    {
		anna_member_create(
		    res, anna_mid_get(memb2->name),
		    anna_member_is_static(memb2), member_type);		
	    }
	    else 
	    {
		anna_member_create_property(
		    res, anna_mid_get(memb2->name),
		    anna_member_is_static(memb2), 
		    member_type,
		    getter ? 0 : -1,
		    setter ? 0 : -1);
	    }
	}
    }

    if(!anna_abides(t1, res))
    {
	debug(
	    D_CRITICAL, 
	    L"Intersected type %ls is not subset of it's defining types, %ls\n",
	    res->name, t1->name);

	hash_remove(&anna_abides_cache, anna_tt_make(t1, res), 0, 0);
	anna_abides_fault_count_internal(t1, res, D_CRITICAL);
	CRASH;
    }
    
    if(!anna_abides(t2, res))
    {
	debug(
	    D_CRITICAL, 
	    L"Intersected type %ls is not subset it's defining types, %ls\n",
	    res->name, t2->name);

	hash_remove(&anna_abides_cache, anna_tt_make(t2, res), 0, 0);
	anna_abides_fault_count_internal(t2, res, D_CRITICAL);
	CRASH;
    }
    
    anna_type_close(res);
}

anna_type_t *anna_type_intersect(anna_type_t *t1, anna_type_t *t2)
{
    if(t2 == null_type)
    {
	return t1;
    }
    if(t1 == null_type)
    {
	return t2;
    }
    if(anna_abides(t1, t2))
    {
	return t2;
    }
    if(anna_abides(t2, t1))
    {
	return t1;
    }

    anna_tt_t tt = 
	{
	    (t1<t2)?t1:t2,(t1<t2)?t2:t1,
	};
    
    anna_type_t *res = hash_get(&anna_intersect_cache, &tt);
    if(res)
    {
	return res;
    }
    
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb,L"!intersection(%ls, %ls)", t1->name, t2->name);
    res = anna_type_create(sb_content(&sb), 0);
    sb_destroy(&sb);
    
    anna_type_intersect_into(res, t1, t2);
    
    if(!anna_abides(res, any_type))
    {
	debug(
	    D_CRITICAL, 
	    L"Type %ls does not abide to the object type. Reasons:\n", res->name);
	anna_abides_fault_count_internal(res, any_type, D_CRITICAL);
	CRASH;
    }
    
    return res;
}

int anna_abides_search(
    anna_node_call_t *call,
    anna_function_type_t **function,
    size_t function_count)
{
    int i, j;
    int match = -1;
    int fault_count=0;

    if(call->child_count == 0)
    {
	if(function_count == 1)
	{
	    return 0;
	}
	else
	{
	    anna_error((anna_node_t *)call, L"Multiple function aliases exists, but no parameters to differentiate\n");
	    return -1;
	}    
    }
    

    for(i=0; i<function_count; i++)
    {
	anna_function_type_t *ft = function[i];	
	
	debug(
	    D_SPAM, L"Check %ls against\n",
	    call->child[0]->return_type->name);

 	if(anna_node_validate_call_parameters(
	       call, ft, 0))
	{
	    debug(
		D_SPAM, L"Params match");
	    int ok = 1;
	    int my_fault_count = 0;
	    anna_node_call_t *call_copy = (anna_node_call_t *)anna_node_clone_shallow((anna_node_t *)call);
	    call_copy->stack = call->stack;
	    anna_node_call_map(call_copy, ft);
	    
	    if(ok)
	    {
		for(j=0; j<call->child_count; j++)
		{
		    int ft_idx = mini(j, ft->input_count-1);
		    if(anna_abides(
			   call_copy->child[j]->return_type, 
			   ft->input_type[ft_idx]))
		    {
			my_fault_count += 
			    anna_abides_fault_count(
				ft->input_type[ft_idx], 
				call_copy->child[j]->return_type);
		    }
		    else
		    {
			ok=0;
			debug(
			    D_SPAM, L"Argument %d, %ls does not match %ls!\n", 
			    j, call_copy->child[j]->return_type->name, 
			    ft->input_type[ft_idx]->name);
		    }
		}
	    }
	    
	    if(ok)
	    {
		debug(D_SPAM, L"Match %d!\n", i);
		
		if((match == -1) || my_fault_count < fault_count)
		{
		    match = i;
		    fault_count = my_fault_count;
		}
	    }
	}
    }
    
    return match;
}


