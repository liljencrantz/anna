#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "anna.h"
#include "anna_float.h"
#include "anna_type.h"
#include "anna_int.h"
#include "anna_member.h"
#include "anna_function.h"
#include "anna_string.h"
#include "anna_vm.h"

#include "anna_float_i.c"

static void anna_float_set(anna_object_t *this, double value)
{
    memcpy(anna_entry_get_addr(this,ANNA_MID_FLOAT_PAYLOAD), &value, sizeof(double));
}

anna_object_t *anna_float_create(double value)
{
    anna_object_t *obj= anna_object_create(float_type);
    anna_float_set(obj, value);
    return obj;
}

double anna_float_get(anna_object_t *this)
{
    double result;
    memcpy(&result, anna_entry_get_addr(this,ANNA_MID_FLOAT_PAYLOAD), sizeof(double));
    return result;
}

static anna_vmstack_t *anna_float_cmp(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 2;
    anna_vmstack_drop(stack, 3);
    if(unlikely( anna_entry_null(param[1])))
    {
        anna_vmstack_push_object(stack, null_object);
    }
    else if(anna_is_float(param[1]))
    {
        double v1 = anna_as_float(param[0]);
	double v2 = anna_as_float(param[1]);
	if(v1 > v2)
	{
	    anna_vmstack_push_entry(stack, anna_from_int(1));
	}
	else if(v1 < v2)
	{
	    anna_vmstack_push_entry(stack, anna_from_int(-1));
	}
	else{
	    anna_vmstack_push_entry(stack, anna_from_int(0));
	}   
    }
    else if(anna_is_int(param[1]))
    {
        double v1 = anna_as_float(param[0]);
	double v2 = (double)anna_as_int(param[1]);
	if(v1 > v2)
	{
	    anna_vmstack_push_entry(stack, anna_from_int(1));
	}
	else if(v1 < v2)
	{
	    anna_vmstack_push_entry(stack, anna_from_int(-1));
	}
	else{
	    anna_vmstack_push_entry(stack, anna_from_int(0));
	}   
    }
    else
    {	
        anna_vmstack_push_object(stack, null_object);
    }

    return stack;
}

static anna_vmstack_t *anna_float_to_string(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 1;
    anna_vmstack_drop(stack, 2);
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"%f", anna_as_float(param[0]));
    wchar_t *buff = sb_content(&sb);
    wchar_t *comma = wcschr(buff, ',');
    if(comma)
    {
	*comma = '.';
    }
    anna_vmstack_push_object(stack, anna_string_create(sb_length(&sb), buff));
    return stack;
}

static anna_vmstack_t *anna_float_hash(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 1;
    anna_vmstack_drop(stack, 2);
    double cmp = anna_as_float(param[0]);
    int res = anna_hash((int *)&cmp, sizeof(double) / sizeof(int));
     anna_vmstack_push_int(
	stack,
	res);
    return stack;
}

void anna_float_type_create(anna_stack_template_t *stack)
{
    anna_type_t *argv[] = 
	{
	    float_type, object_type
	}
    ;
    wchar_t *argn[]=
	{
	    L"this", L"other"
	}
    ;
    
    anna_member_create_blob(
	float_type, ANNA_MID_FLOAT_PAYLOAD,  L"!floatPayload",
	0, sizeof(double));
    
    anna_native_method_create(
	float_type,
	-1,
	L"__cmp__",
	0,
	&anna_float_cmp, 
	int_type,
	2, argv, argn);    
    
    anna_native_method_create(
	float_type,
	ANNA_MID_TO_STRING,
	L"toString",
	0,
	&anna_float_to_string, 
	string_type, 1, argv, argn);
    
    anna_native_method_create(
	float_type,
	ANNA_MID_HASH_CODE,
	L"hashCode",
	0,
	&anna_float_hash, 
	int_type, 1, argv, argn);
    
    anna_float_type_i_create(stack);
    
}
