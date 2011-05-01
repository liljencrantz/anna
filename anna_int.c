#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_int.h"
#include "anna_type.h"
#include "anna_member.h"
#include "anna_function.h"
#include "anna_string.h"
#include "anna_vm.h"

#include "anna_int_i.c"

static void anna_int_set(anna_object_t *this, int value)
{
    memcpy(anna_member_addr_get_mid(this,ANNA_MID_INT_PAYLOAD), &value, sizeof(int));
}

anna_object_t *anna_int_create(int value)
{
    anna_object_t *obj= anna_object_create(int_type);
    anna_int_set(obj, value);
    return obj;
}

int anna_int_get(anna_object_t *this)
{
    int result;
    memcpy(&result, anna_member_addr_get_mid(this,ANNA_MID_INT_PAYLOAD), sizeof(int));
    return result;
}

static anna_vmstack_t *anna_int_init(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;
    //wprintf(L"LALALA %d %d\n", param[0], param[1]);
    anna_int_set(anna_as_obj(param[0]), anna_as_int(param[1]));
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push_object(stack, anna_as_obj(param[0]));
    return stack;
}

static anna_vmstack_t *anna_int_hash(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 1;
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push_entry(stack, param[0]);
    return stack;
}

static anna_vmstack_t *anna_int_cmp(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;

    anna_entry_t *res;
    if(unlikely(anna_is_obj(param[1]) && anna_as_obj(param[1]) == null_object))
    {
	res = anna_from_obj(null_object);
    }
    else
    {
	res =  anna_from_int(anna_as_int(param[0]) - anna_as_int(param[1]));
    }
    
    anna_vmstack_drop(stack, 3);
    anna_vmstack_push_entry(stack, res);
    return stack;
}

static anna_vmstack_t *anna_int_to_string(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 1;

    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"%d", anna_as_int(param[0]));
    
    anna_vmstack_drop(stack, 2);
    anna_vmstack_push_object(stack, anna_string_create(sb_length(&sb), sb_content(&sb)));
    sb_destroy(&sb);
    return stack;
}

void anna_int_type_create(anna_stack_template_t *stack)
{
    anna_type_t *i_argv[] = 
	{
	    int_type, object_type
	}
    ;
    wchar_t *i_argn[]=
	{
	    L"this", L"other"
	}
    ;
    
    anna_type_t *ii_argv[] = 
	{
	    int_type, int_type
	}
    ;
    wchar_t *ii_argn[]=
	{
	    L"this", L"other"
	}
    ;
    
    anna_member_create(
	int_type,
	ANNA_MID_INT_PAYLOAD, 
	L"!intPayload", 
	0,
	null_type);
    
    anna_native_method_create(
	int_type,
	-1,
	L"__init__",
	0,
	&anna_int_init, 
	object_type,
	2, ii_argv, ii_argn);    
    
    anna_native_method_create(
	int_type,
	-1,
	L"__cmp__",
	0,
	&anna_int_cmp, 
	int_type,
	2, i_argv, i_argn);    
    
    anna_native_method_create(
	int_type,
	ANNA_MID_HASH_CODE,
	L"hashCode",
	0,
	&anna_int_hash, 
	int_type, 1, i_argv, i_argn);
    
    anna_native_method_create(
	int_type,
	ANNA_MID_TO_STRING,
	L"toString",
	0,
	&anna_int_to_string, 
	string_type, 1, i_argv, i_argn);
    
    anna_int_type_i_create(stack);
}
