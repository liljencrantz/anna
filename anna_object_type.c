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
#include "anna_member.h"
#include "anna_mid.h"
#include "anna_object_i.c"

static anna_vmstack_t *anna_object_init(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 1;
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push_object(stack, this);
    return stack;
}

static anna_vmstack_t *anna_object_cmp(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push_entry(stack, anna_from_int(this->type- anna_as_obj(param[1])->type));
    return stack;
}

static anna_vmstack_t *anna_object_hash(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 1;
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push_object(stack, anna_int_create(hash_ptr_func(this->type)));
    return stack;
}

static anna_vmstack_t *anna_object_to_string(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 1;
    anna_object_t *this = anna_as_obj_fast(param[0]);
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"Object of type %ls", this->type->name);
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push_object(stack, anna_string_create(sb_length(&sb), sb_content(&sb)));
    return stack;
}

static anna_vmstack_t *anna_object_type(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 1;
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push_object(stack, anna_type_wrap(this->type));
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
    anna_member_create_native_method(
	object_type, anna_mid_get(L"__init__"),
	0, &anna_object_init, object_type, 1,
	argv, argn);
    
    anna_member_create_native_method(
	object_type, ANNA_MID_HASH_CODE, 0,
	&anna_object_hash, int_type, 1, argv,
	argn);
    anna_member_document(
	object_type,
	ANNA_MID_HASH_CODE,
	L"Hash function. Should return the same number for two identical objects.");

    anna_member_create_native_method(
	object_type,
	ANNA_MID_CMP,
	0,
	&anna_object_cmp,
	int_type,
	2,
	argv,
	argn);
    anna_member_document(
	object_type,
	ANNA_MID_CMP,
	L"Comparison method. Should return a negative number, zero or a positive number if the compared object is smaller than, equal to or greater than the object being called, respectively. If the objects can't be compared, null should be returned.");
    
    anna_member_create_native_method(
	object_type,
	ANNA_MID_TO_STRING,
	0,
	&anna_object_to_string,
	string_type,
	1,
	argv,
	argn);
    anna_member_document(
	object_type,
	ANNA_MID_TO_STRING,
	L"String conversion. Called by the String.convert method.");
    
    anna_member_create_native_property(
	object_type, anna_mid_get(L"__type__"),
	type_type, &anna_object_type, 0);
    
    anna_object_type_i_create();
    anna_type_object_is_created();

}

