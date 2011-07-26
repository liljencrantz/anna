#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <locale.h>

#include "common.h"
#include "util.h"
#include "anna_function.h"
#include "anna.h"
#include "anna_module.h"
#include "clib/lang/int.h"
#include "clib/anna_function_type.h"
#include "anna_type.h"
#include "anna_member.h"
#include "anna_status.h"
#include "anna_vm.h"
#include "anna_tt.h"
#include "anna_alloc.h"
#include "anna_slab.h"
#include "anna_mid.h"
#include "anna_attribute.h"

#define ABIDES_IN_TRANSIT -1

static hash_table_t anna_abides_cache;
static hash_table_t anna_intersect_cache;

static int anna_abides_verbose=0;

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
	if(verbose)
	    debug(verbose, L"Input count mismatch\n");
	return 0;
    }
    
    if(check_type)
    {
	for(i=!!is_method; i<contender->input_count; i++)
	{
	    if(!anna_abides(contender->input_type[i], role_model->input_type[i]))
	    {
		if(verbose)
		    debug(verbose, L"Input %d mismatches, %ls does not abide to %ls\n", 
			  i,
			  contender->input_type[i]->name,
			  role_model->input_type[i]->name );
		return 0;
	    }
	}
    }
    for(i=!!is_method; i<contender->input_count; i++)
    {
	if(wcscmp(contender->input_name[i], role_model->input_name[i]) != 0)
	{
	    if(verbose)
		debug(verbose, L"Input %d mismatches on name %ls != %ls\n", i, role_model->input_name[i], contender->input_name[i]);
	    	    
	    return 0;
	}
    }
    
    if(check_type)
    {
	if(!anna_abides(contender->return_type, role_model->return_type))
	{
	    if(verbose)
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
    if(anna_abides_verbose)
	debug(D_ERROR,L"Check if type %ls abides to %ls\n", contender->name, role_model->name);
    
    if(contender == role_model || contender == null_type)
    {
	return 0;
    }

    anna_tt_t tt = 
	{
	    contender, role_model
	}
    ;
    
    if(!verbose)
    {
	long count = (long)hash_get(&anna_abides_cache, &tt);
	if(count == ABIDES_IN_TRANSIT)
	{
	    //wprintf(L"Check %ls as %ls, in transit\n", contender->name, role_model->name);
	    return 0;
	}
	else if(count != 0)
	{
	    //   wprintf(L"Check %ls as %ls, %d fauls\n", contender->name, role_model->name, count - 1);
	    return count - 1;
	}
    }
    
    static int level = 0;

    level++;

    hash_put(&anna_abides_cache, anna_tt_make(contender, role_model), (void *)(long)ABIDES_IN_TRANSIT);
    
    size_t i;
    int res = 0;    

    if(!contender)
    {
	CRASH;
    }
    
    if(!role_model)
    {
	CRASH;
    }
    /*  
    if(level==1)
    */
    //debug(D_ERROR,L"Check if type %ls abides to %ls\n", contender->name, role_model->name);
    //debug(D_ERROR,L"Role model %ls has %d members\n", role_model->name, role_model->member_count+role_model->static_member_count);
    wchar_t **members = calloc(sizeof(wchar_t *), hash_get_count(&role_model->name_identifier));
    anna_type_get_member_names(role_model, members);    
    
    for(i=0; i<hash_get_count(&role_model->name_identifier); i++)
    {
	assert(members[i]);
	if(wcscmp(members[i], L"__init__") == 0)
	    continue;
	if(wcscmp(members[i], L"__del__") == 0)
	    continue;
	anna_member_t *c_memb = anna_member_get(
	    contender, 
	    anna_mid_get(members[i]));	
	anna_member_t *r_memb = anna_member_get(
	    role_model, 
	    anna_mid_get(members[i]));
	int ok=1;
	if(!c_memb)
	{
	    ok=0;
/*	    if(!ok && level==1)
*/
//	    if(verbose)
	    if(verbose)
		debug(verbose, L"No member named %ls\n", members[i]);
	}
	else if(r_memb->is_bound_method != c_memb->is_bound_method)
	{
	    ok=0;
/*	    if(!ok && level==1)
*/
//	    if(verbose)
	    if(verbose)
		debug(verbose, L"Miss on %ls because of one is a method and not the other\n", members[i]);
	}
	else if(r_memb->is_static != c_memb->is_static)
	{
	    if(verbose)
		debug(verbose, L"Miss on %ls because of one is static and not the other\n", members[i]);
	    ok=0;
	}
	else if(r_memb->is_bound_method)
	{
	    ok = anna_abides_function(
		anna_function_type_unwrap(c_memb->type),
		anna_function_type_unwrap(r_memb->type),
		1,
		1,
		verbose);
/*
	    if(!ok && level==1)
*/
//	    if(verbose)
	    if(!ok && verbose)
		debug(verbose, L"Miss on %ls because of method signature mismatch\n", members[i]);
	}
	else
	{
	    ok = anna_abides(c_memb->type, r_memb->type);
/*	    
	    if(!ok && level==1)
*/
//	    if(verbose)
	    if(!ok && verbose)
		debug(verbose, L"Miss on %ls because of %ls\n", members[i], c_memb->type?L"incompatibility":L"missing member");
	}

	res += !ok;
    }
    free(members);
/*
    if(level==1)
	wprintf(L"%ls abides to %ls: %ls\n", contender->name, role_model->name, res==0?L"true": L"false");
*/  
    hash_put(&anna_abides_cache, anna_tt_make(contender, role_model), (void *)(long)(res+1));
    level--;
    

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

    wchar_t **members = calloc(sizeof(wchar_t *), hash_get_count(&t2->name_identifier));
    anna_type_get_member_names(t2, members);    
    
    for(i=0; i<hash_get_count(&t2->name_identifier); i++)
    {
	if(wcscmp(members[i], L"__init__") == 0)
	    continue;
	if(wcscmp(members[i], L"__del__") == 0)
	    continue;

	anna_member_t *memb1 = anna_member_get(
	    t1, 
	    anna_mid_get(members[i]));	
	anna_member_t *memb2 = anna_member_get(
	    t2, 
	    anna_mid_get(members[i]));
	if(!memb1)
	{
//	    wprintf(L"Skip %ls\n", members[i]);
	    continue;
	}
	else if(memb1->is_bound_method != memb2->is_bound_method)
	{
//	    wprintf(L"Skip %ls\n", members[i]);
	    continue;
	}
	else if(memb1->is_static != memb2->is_static)
	{
//	    wprintf(L"Skip %ls\n", members[i]);
	    continue;
	}
	else if(memb2->is_bound_method)
	{
	    anna_function_type_t *ft1 = anna_function_type_unwrap(memb1->type);
	    anna_function_type_t *ft2 = anna_function_type_unwrap(memb2->type);
	    
	    if(
		anna_abides_function(
		    ft1, ft2, 1, 0, 0)) 
	    {
		anna_type_t **types = malloc(sizeof(anna_type_t *)*ft2->input_count);
		int i;
		for(i=0; i<ft2->input_count; i++)
		{
		    types[i] = anna_type_intersect(ft1->input_type[i], ft2->input_type[i]);		    
		}
		
		anna_member_create_native_method(
		    res,
		    anna_mid_get(memb2->name),
		    ft2->flags,
		    &anna_vm_null_function,
		    anna_type_intersect(ft1->return_type,ft2->return_type),
		    ft2->input_count,
		    types,
		    ft2->input_name);
		free(types);
		
		anna_function_t *new_fun = 
		    anna_function_unwrap(
			anna_as_obj_fast(
			    anna_entry_get_static(
				res, 
				anna_mid_get(
				    memb2->name))));
		
		anna_function_t *ff1 = 
		    anna_function_unwrap(
			anna_as_obj_fast(
			    anna_entry_get_static(
				t1, 
				anna_mid_get(
				    memb2->name))));
		
		anna_function_t *ff2 = 
		    anna_function_unwrap(
			anna_as_obj_fast(
			    anna_entry_get_static(
				t2, 
				anna_mid_get(
				    memb2->name))));
		
		array_list_t alias = AL_STATIC;
		anna_attribute_call_all(ff1->attribute, L"alias", &alias);

		for(i=0; i<al_get_count(&alias); i++)
		{
		    anna_node_t *al = al_get(&alias, i);

		    if(al->node_type == ANNA_NODE_IDENTIFIER)
		    {
			anna_node_identifier_t *nam = (anna_node_identifier_t *)al;
			if(anna_attribute_has_alias(ff2->attribute, nam->name))
			{
			    anna_function_alias_add(new_fun, nam->name);
			}
		    }
		    
		}
	    }
	}
	else
	{
//	    wprintf(L"Memb %ls\n", members[i]);
	    if(anna_abides(memb1->type, memb2->type) && anna_abides(memb2->type, memb1->type))
	    {
		anna_member_create(
		    res, anna_mid_get(memb2->name),
		    memb2->is_static, memb2->type);
	    }
	}
    }
    free(members);    

    if(!anna_abides(t1, res))
    {
	debug(
	    D_CRITICAL, 
	    L"Intersected type %ls is not subset of it's defining types, %ls\n", res->name, t1->name);

	hash_remove(&anna_abides_cache, anna_tt_make(t1, res), 0, 0);
	
	anna_abides_fault_count_internal(t1, res, D_CRITICAL);
	CRASH;
    }
    
    if(!anna_abides(t2, res))
    {
	debug(
	    D_CRITICAL, 
	    L"Intersected type %ls is not subset it's defining types, %ls\n", res->name, t2->name);
	anna_abides_fault_count_internal(t2, res, D_CRITICAL);
	CRASH;
    }
    

}

anna_type_t *anna_type_intersect(anna_type_t *t1, anna_type_t *t2)
{
//    wprintf(L"\n\n\nSTART %ls vs %ls\n\n\n\n", t1->name, t2->name);
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
	}
    ;
    
    anna_type_t *res = hash_get(&anna_intersect_cache, &tt);
    if(res)
    {
	return res;
    }
    
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb,L"!intersection(%ls,%ls)", t1->name, t2->name);
    res = anna_type_create(sb_content(&sb), 0);
    sb_destroy(&sb);
    
    anna_type_intersect_into(res, t1, t2);
    
    if(!anna_abides(res, object_type))
    {
	debug(
	    D_CRITICAL, 
	    L"Type %ls does not abide to the object type. Reasons:\n", res->name);
	anna_abides_fault_count_internal(res, object_type, D_CRITICAL);
	CRASH;
    }
    
    return res;
}


