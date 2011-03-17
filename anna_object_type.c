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

static anna_object_t *anna_object_init(anna_object_t **param)
{
    return param[0];    
}


void anna_object_type_create()
{

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
	&anna_object_init, 
	complex_type, 1, argv, argn);

}
