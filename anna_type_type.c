#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_type.h"
#include "anna_type_type.h"
#include "anna_string.h"
#include "anna_list.h"
#include "anna_member.h"
#include "anna_node_create.h"
#include "anna_vm.h"
#include "anna_int.h"

static inline anna_object_t *anna_type_to_string_i(anna_object_t **param)
{
    anna_type_t *type = anna_type_unwrap(param[0]);
    return anna_string_create(wcslen(type->name), type->name);
}
ANNA_VM_NATIVE(anna_type_to_string, 1)

static inline anna_object_t *anna_type_i_get_member_i(anna_object_t **param)
{
    anna_object_t *lst = anna_list_create(member_type);
    int i;
    anna_type_t *type = anna_type_unwrap(param[0]);

    wchar_t **member_name = malloc(sizeof(wchar_t *)*anna_type_member_count(type));
    anna_type_get_member_names(type, member_name);
    for(i=0;i<anna_type_member_count(type); i++)
    {
	anna_list_add(
	    lst,
	    anna_member_wrap(
		type,
		anna_type_member_info_get(
		    type,
		    member_name[i])));	
    }
    
    return lst;
}
ANNA_VM_NATIVE(anna_type_i_get_member, 1)

static anna_vmstack_t *anna_type_cmp(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 2;
    anna_type_t *type1 = anna_type_unwrap(param[0]);
    anna_type_t *type2 = anna_type_unwrap(param[1]);
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push(stack, anna_int_create(type1-type2));
    return stack;
}

static anna_vmstack_t *anna_type_abides(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 2;
    anna_type_t *type1 = anna_type_unwrap(param[0]);
    anna_type_t *type2 = anna_type_unwrap(param[1]);
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push(stack, anna_abides(type1, type2)?anna_int_one:null_object);
    return stack;
}

static anna_vmstack_t *anna_type_hash(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 1;
    anna_vmstack_drop(stack, 2);
    anna_type_t *type = anna_type_unwrap(param[0]);
    anna_vmstack_push(stack, anna_int_create(hash_ptr_func(type)));
    return stack;
}

void anna_type_type_create(anna_stack_template_t *stack)
{
    anna_member_create(
	type_type,
	ANNA_MID_TYPE_WRAPPER_PAYLOAD,
	L"!typeWrapperPayload",
	0,
	null_type
	);
}


void anna_type_type_create2(anna_stack_template_t *stack)
{
    anna_type_t *argv[] = 
	{
	    type_type, type_type
	}
    ;
    wchar_t *argn[]=
	{
	    L"this", L"other"
	}
    ;

    anna_native_property_create(
	type_type,
	-1,
	L"name",
	string_type,
	&anna_type_to_string, 
	0);
    
    anna_native_property_create(
	type_type,
	-1,
	L"member",
	anna_list_type_get(member_type),
	&anna_type_i_get_member, 
	0);

    anna_native_method_create(
	type_type,
	ANNA_MID_HASH_CODE,
	L"hashCode",
	0,
	&anna_type_hash, 
	int_type, 1, argv, argn);
    
    anna_native_method_create(
	type_type,
	ANNA_MID_CMP,
	L"__cmp__",
	0,
	&anna_type_cmp, 
	object_type, 2, argv, argn);
    
    anna_native_method_create(
	type_type,
	ANNA_MID_TO_STRING,
	L"toString",
	0,
	&anna_type_to_string, 
	string_type, 1, argv, argn);    

    anna_native_method_create(
	type_type,
	-1,
	L"abides",
	0,
	&anna_type_abides, 
	object_type, 2, argv, argn);
    
}
