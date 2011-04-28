#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <complex.h>

#include "anna.h"
#include "anna_object_type.h"
#include "anna_function.h"
#include "anna_int.h"
#include "anna_vm.h"
#include "anna_string.h"
#include "anna_type.h"

#include "anna_object_i.c"

static anna_vmstack_t *anna_object_init(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 1;
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push_object(stack, param[0]);
    return stack;
}

static anna_vmstack_t *anna_object_cmp(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 2;
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push_object(stack, anna_int_create(param[0]->type-param[1]->type));
    return stack;
}

static anna_vmstack_t *anna_object_hash(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 1;
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push_object(stack, anna_int_create(hash_ptr_func(param[0]->type)));
    return stack;
}

static anna_vmstack_t *anna_object_to_string(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 1;
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"Object of type %ls", param[0]->type->name);
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push_object(stack, anna_string_create(sb_length(&sb), sb_content(&sb)));
    return stack;
}

void anna_object_type_create()
{

    anna_type_t *argv[] = 
	{
	    object_type, object_type
	}
    ;
    
    wchar_t *argn[]=
	{
	    L"this", L"other"
	}
    ;
/*    
    mid_t mmid;
    anna_function_t *fun;
*/  
    anna_native_method_create(
	object_type,
	-1,
	L"__init__",
	0,
	&anna_object_init, 
	object_type, 1, argv, argn);
    
    anna_native_method_create(
	object_type,
	ANNA_MID_HASH_CODE,
	L"hashCode",
	0,
	&anna_object_hash, 
	int_type, 1, argv, argn);
    
    anna_native_method_create(
	object_type,
	ANNA_MID_CMP,
	L"__cmp__",
	0,
	&anna_object_cmp, 
	object_type, 2, argv, argn);
    
    anna_native_method_create(
	object_type,
	ANNA_MID_TO_STRING,
	L"toString",
	0,
	&anna_object_to_string, 
	string_type, 1, argv, argn);
    
    anna_object_type_i_create();
    anna_type_object_is_created();
}
