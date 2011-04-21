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

anna_object_t *anna_float_create(double value)
{
    anna_object_t *obj= anna_object_create(float_type);
    anna_float_set(obj, value);
    return obj;
}

void anna_float_set(anna_object_t *this, double value)
{
    memcpy(anna_member_addr_get_mid(this,ANNA_MID_FLOAT_PAYLOAD), &value, sizeof(double));
}

double anna_float_get(anna_object_t *this)
{
    double result;
    memcpy(&result, anna_member_addr_get_mid(this,ANNA_MID_FLOAT_PAYLOAD), sizeof(double));
    return result;
}

static anna_vmstack_t *anna_float_cmp(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 2;
    anna_vmstack_drop(stack, 3);
    if(unlikely(param[1]->type != float_type))
    {
        anna_vmstack_push(stack, null_object);
        return stack;
    }  

    double v1 = anna_float_get(param[0]);
    double v2 = anna_float_get(param[1]);
    if(v1 > v2)
    {
	anna_vmstack_push(stack, anna_int_one);
    }
    else if(v1 < v2)
    {
	anna_vmstack_push(stack, anna_int_minus_one);
    }
    else{
	anna_vmstack_push(stack, anna_int_zero);
    }    
    return stack;
}

static anna_vmstack_t *anna_float_to_string(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_object_t **param = stack->top - 1;
    anna_vmstack_drop(stack, 2);
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"%f", anna_float_get(param[0]));
    anna_vmstack_push(stack, anna_string_create(sb_length(&sb), sb_content(&sb)));
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

    anna_member_create(
	float_type, ANNA_MID_FLOAT_PAYLOAD,  L"!floatPayload",
	0, null_type);
    /*
      If we can't fit a double into a void * on this platform, add a
      second dummy payload. This assumes that sizeof(double) is never
      more than twice the size of sizeof(void *).

      This code also assumes that consecutively registered members
      will be stored next to each other, which is currently true, but
      we will need to keep this in mind when trying to optimize member
      layout in the future.
    */
    if(sizeof(double) > sizeof(void *)) 
    {
	anna_member_create(
	    float_type, -1,  L"!floatPayload2", 0, 
	    null_type);
    }
    
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
    
    anna_float_type_i_create(stack);
    
}
