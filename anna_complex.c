#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <complex.h>

#include "anna.h"
#include "anna_complex.h"
#include "anna_type.h"
#include "anna_int.h"
#include "anna_float.h"
#include "anna_member.h"
#include "anna_function.h"
#include "anna_string.h"
#include "anna_vm.h"

#include "anna_complex_i.c"

static inline void anna_complex_set(anna_object_t *this, complex double value)
{
    size_t off = this->type->mid_identifier[ANNA_MID_COMPLEX_PAYLOAD]->offset;
    memcpy(&this->member[off], &value, sizeof(complex double));
}

anna_object_t *anna_complex_create(complex double value)
{
    anna_object_t *obj= anna_object_create(complex_type);
    anna_complex_set(obj, value);
    return obj;
}

inline complex double anna_complex_get(anna_object_t *this)
{
    complex double result;
    size_t off = this->type->mid_identifier[ANNA_MID_COMPLEX_PAYLOAD]->offset;
    memcpy(&result, &this->member[off], sizeof(complex double));
    return result;
}

static anna_vmstack_t *anna_complex_init(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 3;
    complex double result = anna_as_float(param[1]) + I * anna_as_float(param[2]);
    size_t off = anna_as_obj(param[0])->type->mid_identifier[ANNA_MID_COMPLEX_PAYLOAD]->offset;
    memcpy(&anna_as_obj(param[0])->member[off], &result, sizeof(complex double));
    anna_vmstack_drop(stack, 4);
    anna_vmstack_push_entry(stack, param[0]);
    return stack;
}

static anna_vmstack_t *anna_complex_to_string(anna_vmstack_t *stack, anna_object_t *me)
{
    anna_entry_t **param = stack->top - 1;
    complex double val = anna_as_complex(param[0]);
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"%f + i%f", creal(val), cimag(val));

    wchar_t *buff = sb_content(&sb);
    wchar_t *comma = wcschr(buff, ',');
    if(comma)
    {
	*comma = '.';
	comma = wcschr(comma+1, ',');
	if(comma)
	{
	    *comma = '.';
	}
    }

    anna_vmstack_drop(stack, 2);
    anna_vmstack_push_object(stack, anna_string_create(sb_length(&sb), buff));
    return stack;
}

void anna_complex_type_create(anna_stack_template_t *stack)
{

    anna_member_create_blob(
	complex_type, ANNA_MID_COMPLEX_PAYLOAD,  L"!complexPayload",
	0, sizeof(complex double));

    anna_type_t *argv[] = 
	{
	    complex_type,
	    float_type,
	    float_type
	}
    ;
    
    wchar_t *argn[]=
	{
	    L"this", L"real", L"imag"
	}
    ;

    anna_native_method_create(
	complex_type,
	-1,
	L"__init__",
	0,
	&anna_complex_init, 
	complex_type,
	3, argv, argn);
    
    anna_native_method_create(
	complex_type,
	ANNA_MID_TO_STRING,
	L"toString",
	0,
	&anna_complex_to_string, 
	string_type, 1, argv, argn);    

    anna_complex_type_i_create(stack);
}
