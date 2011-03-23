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

#include "anna_complex_i.c"

anna_object_t *anna_complex_create(complex double value)
{
    anna_object_t *obj= anna_object_create(complex_type);
    anna_complex_set(obj, value);
    return obj;
}

inline void anna_complex_set(anna_object_t *this, complex double value)
{
    size_t off = this->type->mid_identifier[ANNA_MID_COMPLEX_PAYLOAD]->offset;
    memcpy(&this->member[off], &value, sizeof(complex double));
}

inline complex double anna_complex_get(anna_object_t *this)
{
    complex double result;
    size_t off = this->type->mid_identifier[ANNA_MID_COMPLEX_PAYLOAD]->offset;
    memcpy(&result, &this->member[off], sizeof(complex double));
    return result;
}

static anna_object_t *anna_complex_init(anna_object_t **param)
{
    complex double result = anna_float_get(param[1]) + I * anna_float_get(param[2]);
    size_t off = param[0]->type->mid_identifier[ANNA_MID_COMPLEX_PAYLOAD]->offset;
    memcpy(&param[0]->member[off], &result, sizeof(complex double));
    return param[0];
}

static anna_object_t *anna_complex_to_string(anna_object_t **param)
{
    complex double val = anna_complex_get(param[0]);
    string_buffer_t sb = SB_STATIC;
    sb_printf(&sb, L"%f + i%f", creal(val), cimag(val));
    return anna_string_create(sb_length(&sb), sb_content(&sb));
}

void anna_complex_type_create(anna_stack_template_t *stack)
{

    anna_member_create(
	complex_type, ANNA_MID_COMPLEX_PAYLOAD,  L"!complexPayload",
	0, null_type);
    anna_member_create(
	complex_type, -1,  L"!complexPayload2", 0, 
	null_type);
    /*
      If we can't fit a complex into a void * on this platform, add a
      second dummy payload. This assumes that sizeof(complex) is never
      more than twice the size of sizeof(void *).

      This code also assumes that consecutively registered members
      will be stored next to each other, which is currently true, but
      we will need to keep this in mind when trying to optimize member
      layout in the future.
    */
    if(sizeof(complex double) > sizeof(void *)*2) 
    {
	anna_member_create(
	    complex_type, -1,  L"!complexPayload3", 0, 
	    null_type);
	anna_member_create(
	    complex_type, -1,  L"!complexPayload4", 0, 
	    null_type);
    }

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
