#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_pair.h"
#include "anna_int.h"
#include "anna_stack.h"
#include "anna_type.h"
#include "anna_member.h"
#include "anna_function_type.h"
#include "anna_function.h"
#include "anna_range.h"
#include "anna_vm.h"
#include "anna_tt.h"
#include "anna_mid.h"

#include "anna_macro.h"

static hash_table_t anna_pair_specialization;

anna_object_t *anna_pair_create(anna_entry_t *first, anna_entry_t *second)
{
    anna_object_t *obj= anna_object_create(anna_pair_type_get(anna_as_obj(first)->type, anna_as_obj(second)->type));
    anna_pair_set_first(obj, first);
    anna_pair_set_second(obj, second);
    return obj;
}

ANNA_VM_NATIVE(anna_pair_init, 3)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_pair_set_first(this, param[1]);
    anna_pair_set_second(this, param[2]);
    return param[0];
}

ANNA_VM_NATIVE(anna_pair_get_first_i, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    return anna_pair_get_first(this);
}

ANNA_VM_NATIVE(anna_pair_get_second_i, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    return anna_pair_get_second(this);
}

ANNA_VM_NATIVE(anna_pair_set_first_i, 2)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_pair_set_first(this, param[1]);
    return param[1];
}

ANNA_VM_NATIVE(anna_pair_set_second_i, 2)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_pair_set_second(this, param[1]);
    return param[1];
}
/*
static anna_type_t *anna_pair_get_specialization1(anna_object_t *obj)
{
    return *((anna_type_t **)
	     anna_entry_get_addr(
		 obj,
		 ANNA_MID_PAIR_SPECIALIZATION1));    
}

static anna_type_t *anna_pair_get_specialization2(anna_object_t *obj)
{
    return *((anna_type_t **)
	     anna_entry_get_addr(
		 obj,
		 ANNA_MID_PAIR_SPECIALIZATION2));    
}
*/
static void anna_pair_type_create_internal(
    anna_type_t *type, 
    anna_type_t *spec1,
    anna_type_t *spec2)
{
    anna_member_create(
	type, ANNA_MID_PAIR_FIRST, 0, null_type);
    anna_member_create(
	type, ANNA_MID_PAIR_SECOND, 0,
	null_type);
    anna_member_create(
	type,
	ANNA_MID_PAIR_SPECIALIZATION1,
	1,
	null_type);
    anna_member_create(
	type,
	ANNA_MID_PAIR_SPECIALIZATION2,
	1,
	null_type);
    
    (*(anna_type_t **)anna_entry_get_addr_static(type,ANNA_MID_PAIR_SPECIALIZATION1)) = spec1;
    (*(anna_type_t **)anna_entry_get_addr_static(type,ANNA_MID_PAIR_SPECIALIZATION2)) = spec2;
    
    anna_type_t *argv[] = 
	{
	    type,
	    spec1,
	    spec2
	}
    ;
    
    wchar_t *argn[]=
	{
	    L"this", L"second", L"first"
	}
    ;

    anna_member_create_native_method(
	type, anna_mid_get(L"__init__"), 0,
	&anna_pair_init, type, 3, argv, argn);    
    
    
    anna_member_create_native_property(
	type, anna_mid_get(L"first"), spec1,
	&anna_pair_get_first_i,
	&anna_pair_set_first_i, 0);
    anna_member_create_native_property(
	type,
	anna_mid_get(L"second"),
	spec2,
	&anna_pair_get_second_i,
	&anna_pair_set_second_i, 0);
}

static inline void anna_pair_internal_init()
{
    static int init = 0;
    if(likely(init))
	return;
    init=1;
    hash_init(&anna_pair_specialization, hash_tt_func, hash_tt_cmp);
}

void anna_pair_type_create()
{
    anna_pair_internal_init();
    hash_put(&anna_pair_specialization, anna_tt_make(object_type, object_type), pair_type);
    anna_pair_type_create_internal(pair_type, object_type, object_type);
}

anna_type_t *anna_pair_type_get(anna_type_t *subtype1, anna_type_t *subtype2)
{
    anna_pair_internal_init();
    anna_tt_t tt = 
	{
	    subtype1, subtype2
	}
    ;
    
    anna_type_t *spec = hash_get(&anna_pair_specialization, &tt);
    if(!spec)
    {
	string_buffer_t sb;
	sb_init(&sb);
	sb_printf(&sb, L"Pair«%ls,%ls»", subtype1->name, subtype2->name);
	spec = anna_type_native_create(sb_content(&sb), stack_global);
	sb_destroy(&sb);
	hash_put(&anna_pair_specialization, anna_tt_make(subtype1, subtype2), spec);
	anna_pair_type_create_internal(spec, subtype1, subtype2);
	spec->flags |= ANNA_TYPE_SPECIALIZED;
	anna_type_copy_object(spec);
    }
    return spec;
}
