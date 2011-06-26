#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include "common.h"
#include "util.h"
#include "wutil.h"
#include "anna.h"
#include "anna_module.h"
#include "anna_vm.h"
#include "anna_math.h"

ANNA_NATIVE(anna_math_sin, 1)
{
    return anna_entry_null(param[0])? null_entry : anna_from_float(sin(anna_as_float(param[0])));
}

ANNA_NATIVE(anna_math_cos, 1)
{
    return anna_entry_null(param[0])? null_entry : anna_from_float(cos(anna_as_float(param[0])));
}

ANNA_NATIVE(anna_math_tan, 1)
{
    return anna_entry_null(param[0])? null_entry : anna_from_float(tan(anna_as_float(param[0])));
}

ANNA_NATIVE(anna_math_asin, 1)
{
    return anna_entry_null(param[0])? null_entry : anna_from_float(asin(anna_as_float(param[0])));
}

ANNA_NATIVE(anna_math_acos, 1)
{
    return anna_entry_null(param[0])? null_entry : anna_from_float(acos(anna_as_float(param[0])));
}

ANNA_NATIVE(anna_math_atan, 1)
{
    return anna_entry_null(param[0])? null_entry : anna_from_float(atan(anna_as_float(param[0])));
}

ANNA_NATIVE(anna_math_log, 1)
{
    return anna_entry_null(param[0])? null_entry : anna_from_float(log(anna_as_float(param[0])));
}

void anna_math_load(anna_stack_template_t *stack)
{
    anna_module_const_float(stack, L"pi", M_PI, 0);
    anna_module_const_float(stack, L"e", M_E, 0);
    
    wchar_t *f_argn[]={L"value"};
    anna_type_t *f_argv[] = {float_type};
    
    anna_module_function(
	stack, L"sin", 
	0, &anna_math_sin, 
	float_type, 
	1, f_argv, f_argn, 
	L"The sin function computes the sine of value, where value is given in radians.");
    
    anna_module_function(
	stack, L"cos", 
	0, &anna_math_cos, 
	float_type, 
	1, f_argv, f_argn, 
	L"The cos function computes the cosine of value, where value is given in radians.");
    
    anna_module_function(
	stack, L"tan", 
	0, &anna_math_tan, 
	float_type, 
	1, f_argv, f_argn, 
	L"The tan function computes the tangent of value, where value is given in radians.");
    
    anna_module_function(
	stack, L"asin", 
	0, &anna_math_asin, 
	float_type, 
	1, f_argv, f_argn, 
	L"The tan function computes the arc sine of value.");
    
    anna_module_function(
	stack, L"acos", 
	0, &anna_math_acos, 
	float_type, 
	1, f_argv, f_argn, 
	L"The tan function computes the arc cosine of value.");
    
    anna_module_function(
	stack, L"atan", 
	0, &anna_math_atan, 
	float_type,
	1, f_argv, f_argn, 
	L"The atan function computes the arc tangent of value.");
    
    anna_module_function(
	stack, L"log", 
	0, &anna_math_log, 
	float_type,
	1, f_argv, f_argn, 
	L"The log function computes the natural logarithm of value.");
}

