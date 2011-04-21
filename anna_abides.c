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
#include "anna_int.h"
#include "anna_function_type.h"
#include "anna_type.h"
#include "anna_member.h"
#include "anna_status.h"
#include "anna_vm.h"
#include "anna_tt.h"
#include "anna_alloc.h"
#include "anna_slab.h"

#define ABIDES_IN_TRANSIT -1

static hash_table_t anna_abides_cache;

void anna_abides_init()
{
    hash_init(&anna_abides_cache, hash_tt_func, hash_tt_cmp);
}

static int anna_abides_function(
    anna_function_type_t *contender,
    anna_function_type_t *role_model,
    int is_method)
{
    int i;
    if(contender->input_count != role_model->input_count)
    {
	return 0;
    }
    
    for(i=!!is_method; i<contender->input_count; i++)
    {
	if(!anna_abides(contender->input_type[i], role_model->input_type[i]))
	{
	    return 0;
	}
    }

    if(!anna_abides(contender->return_type, role_model->return_type))
    {
	return 0;
    }
    
    return 1;
}

int anna_abides_fault_count(anna_type_t *contender, anna_type_t *role_model)
{
    
    if(contender == role_model || contender == null_type)
    {
	return 0;
    }

    anna_tt_t tt = 
	{
	    contender, role_model
	}
    ;
    
    long count = (long)hash_get(&anna_abides_cache, &tt);
    if(count == ABIDES_IN_TRANSIT)
    {
	return 0;
    }
    else if(count != 0)
    {
	return count - 1;
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
	debug(D_ERROR,L"Check if type %ls abides to %ls\n", contender->name, role_model->name);
    */
    //debug(D_SPAM,L"Role model %ls has %d members\n", role_model->name, role_model->member_count+role_model->static_member_count);
    wchar_t **members = calloc(sizeof(wchar_t *), hash_get_count(&role_model->name_identifier));
    anna_type_get_member_names(role_model, members);    
    
    for(i=0; i<anna_type_member_count(role_model); i++)
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
		wprintf(L"Miss on %ls because of missing member in contender\n", members[i]);
*/
	}
	else if(r_memb->is_method != c_memb->is_method)
	{
	    ok=0;
/*	    if(!ok && level==1)
		wprintf(L"Miss on %ls because of one is a method\n", members[i]);
*/
	}
	else if(r_memb->is_method)
	{
	    ok = anna_abides_function(
		anna_function_type_unwrap(c_memb->type),
		anna_function_type_unwrap(r_memb->type),
		1);
/*
	    if(!ok && level==1)
	    wprintf(L"Miss on %ls because method signature mismatch\n", members[i]);
*/
	}
	else
	{
	    ok = anna_abides(c_memb->type, r_memb->type);
/*	    
	    if(!ok && level==1)
		wprintf(L"Miss on %ls because of %ls\n", members[i], c_memb->type?L"incompatibility":L"missing member");
*/	    
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

int anna_abides(anna_type_t *contender, anna_type_t *role_model)
{
    return !anna_abides_fault_count(contender, role_model);
}

