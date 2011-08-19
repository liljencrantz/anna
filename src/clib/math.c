#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include "../common.h"
#include "../util.h"
#include "../wutil.h"
#include "../anna.h"
#include "../anna_module.h"
#include "../anna_vm.h"
#include "clib/clib.h"

#include "math.h"

ANNA_VM_NATIVE(anna_math_sin, 1)
{
    return anna_entry_null(param[0])? null_entry : anna_from_float(sin(anna_as_float(param[0])));
}

ANNA_VM_NATIVE(anna_math_cos, 1)
{
    return anna_entry_null(param[0])? null_entry : anna_from_float(cos(anna_as_float(param[0])));
}

ANNA_VM_NATIVE(anna_math_tan, 1)
{
    return anna_entry_null(param[0])? null_entry : anna_from_float(tan(anna_as_float(param[0])));
}

ANNA_VM_NATIVE(anna_math_asin, 1)
{
    return anna_entry_null(param[0])? null_entry : anna_from_float(asin(anna_as_float(param[0])));
}

ANNA_VM_NATIVE(anna_math_acos, 1)
{
    return anna_entry_null(param[0])? null_entry : anna_from_float(acos(anna_as_float(param[0])));
}

ANNA_VM_NATIVE(anna_math_atan, 1)
{
    return anna_entry_null(param[0])? null_entry : anna_from_float(atan(anna_as_float(param[0])));
}

ANNA_VM_NATIVE(anna_math_log, 1)
{
    return anna_entry_null(param[0])? null_entry : anna_from_float(log(anna_as_float(param[0])));
}

ANNA_VM_NATIVE(anna_math_log2, 1)
{
    return anna_entry_null(param[0])? null_entry : anna_from_float(log2(anna_as_float(param[0])));
}

ANNA_VM_NATIVE(anna_math_log10, 1)
{
    return anna_entry_null(param[0])? null_entry : anna_from_float(log10(anna_as_float(param[0])));
}

ANNA_VM_NATIVE(anna_math_sqrt, 1)
{
    return anna_entry_null(param[0])? null_entry : anna_from_float(sqrt(anna_as_float(param[0])));
}

void anna_math_load(anna_stack_template_t *stack)
{
    anna_module_const_float(stack, L"pi", M_PI, L"The mathematical constant pi, i.e. the circumference of a circle divided by its diameter.");
    anna_module_const_float(stack, L"e", M_E, L"The mathematical constant e, i.e. the base of the natural logarithm.");
    
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

    anna_module_function(
	stack, L"log2", 
	0, &anna_math_log2, 
	float_type,
	1, f_argv, f_argn, 
	L"The log2 function computes the base 2 logarithm of value.");

    anna_module_function(
	stack, L"log10", 
	0, &anna_math_log10, 
	float_type,
	1, f_argv, f_argn, 
	L"The log10 function computes the base 10 logarithm of value.");

    anna_module_function(
	stack, L"sqrt", 
	0, &anna_math_sqrt, 
	float_type,
	1, f_argv, f_argn, 
	L"The sqrt function computes the square root of value.");
}

