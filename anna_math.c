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
#include "anna_module_data.h"
#include "anna_cio.h"
#include "anna_stack.h"
#include "anna_vm.h"
#include "anna_string.h"
#include "anna_function.h"
#include "anna_buffer.h"
#include "anna_intern.h"
#include "anna_list.h"

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

void anna_math_load(anna_stack_template_t *stack)
{
    anna_module_const_float(stack, L"pi", M_PI);
    anna_module_const_float(stack, L"e", M_E);
    
    wchar_t *f_argn[]={L"value"};
    anna_type_t *f_argv[] = {float_type};
    
    anna_module_function(
	stack, L"sin", 
	0, &anna_math_sin, 
	float_type, 
	1, f_argv, f_argn, 
	L"The sin function comutes the sine of value, where value is given in radians.");
    
    anna_module_function(
	stack, L"cos", 
	0, &anna_math_cos, 
	float_type, 
	1, f_argv, f_argn, 
	L"The cos function comutes the cosine of value, where value is given in radians.");
    
    anna_module_function(
	stack, L"tan", 
	0, &anna_math_tan, 
	float_type, 
	1, f_argv, f_argn, 
	L"The tan function comutes the tangent of value, where value is given in radians.");
    
}

