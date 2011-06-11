#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <float.h>

#include "common.h"
#include "anna.h"
#include "anna_float.h"
#include "anna_type.h"
#include "anna_int.h"
#include "anna_member.h"
#include "anna_function.h"
#include "anna_string.h"
#include "anna_vm.h"
#include "anna_util.h"
#include "anna_mid.h"

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

static inline anna_entry_t *anna_float_convert_string_i(anna_entry_t **param)
{
    if(anna_entry_null(param[0]))
    {
	return anna_from_obj(null_object);
    }
    wchar_t *str = anna_string_payload(anna_as_obj(param[0]));
    if(wcslen(str) != anna_string_get_count(anna_as_obj(param[0])))
    {
	return anna_from_obj(null_object);	
    }
    
    /* Strip any underscores from the string */
    wchar_t *in = str;
    wchar_t *out = str;
    while(*in)
    {
	if(*in == L'_')
	{
	    in++;
	}
	else
	{
	    *out++ = *in++;
	}
    }
    *out = 0;
    
    /* Convert to narrow string and send if to strtod */
    char *nstr = wcs2str(str);
    char *end=0;
    errno=0;
    
    double res = strtod(nstr, &end);
    if(errno || *end != 0)
    {
	return anna_from_obj(null_object);
    }
    return anna_from_float(res);
}
ANNA_VM_NATIVE(anna_float_convert_string, 1)

static inline anna_entry_t *anna_float_convert_float_i(anna_entry_t **param)
{
    return param[0];
}
ANNA_VM_NATIVE(anna_float_convert_float, 1)

static inline anna_entry_t *anna_float_convert_int_i(anna_entry_t **param)
{
    if(anna_entry_null(param[0]))
    {
	return anna_from_obj(null_object);
    }
    mpz_t *int_val = anna_int_unwrap(anna_as_obj(param[0]));
    double res = mpz_get_d(*int_val);
    return anna_from_float(res);
}
ANNA_VM_NATIVE(anna_float_convert_int, 1)

static inline anna_entry_t *anna_float_max_exponent_i(anna_entry_t **param)
{
    return anna_from_int(DBL_MAX_EXP);
}
ANNA_VM_NATIVE(anna_float_max_exponent, 1)

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
    
    anna_member_create_native_method(
	float_type,
	-1,
	L"__cmp__",
	0,
	&anna_float_cmp, 
	int_type,
	2, argv, argn);    
    
    anna_member_create_native_method(
	float_type,
	ANNA_MID_TO_STRING,
	L"toString",
	0,
	&anna_float_to_string, 
	string_type, 1, argv, argn);
    
    anna_member_create_native_method(
	float_type,
	ANNA_MID_HASH_CODE,
	L"hashCode",
	0,
	&anna_float_hash, 
	int_type, 1, argv, argn);
    

    wchar_t *conv_argn[]=
	{
	    L"value"
	}
    ;

    mid_t mmid;
    anna_function_t *fun;

    mmid = anna_member_create_native_type_method(
	float_type,
	-1,
	L"convertString",
	0,
	&anna_float_convert_string, 
	float_type, 1, &string_type, conv_argn);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(float_type, mmid)));
    anna_function_alias_add(fun, L"convert");

    mmid = anna_member_create_native_type_method(
	float_type,
	-1,
	L"convertFloat",
	0,
	&anna_float_convert_float, 
	float_type, 1, &float_type, conv_argn);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(float_type, mmid)));
    anna_function_alias_add(fun, L"convert");

    mmid = anna_member_create_native_type_method(
	float_type,
	-1,
	L"convertInt",
	0,
	&anna_float_convert_int, 
	float_type, 1, &int_type, conv_argn);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(float_type, mmid)));
    anna_function_alias_add(fun, L"convert");
    
    anna_native_property_create(
	float_type,
	-1,
	L"maxExponent",
	int_type,
	&anna_float_max_exponent,
	0);
    


    anna_float_type_i_create(stack);
}
