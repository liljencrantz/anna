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
#include "anna_function.h"

static anna_object_t *anna_object_init(anna_object_t **param)
{
    return param[0];    
}

static anna_object_t *anna_object_eq(anna_object_t **param)
{
    return wcscmp(param[0]->type->name, param[1]->type->name)==0;
}

void anna_object_type_create()
{

    anna_type_t *argv[] = 
	{
	    object_type, object_type
	}
    ;
    
    wchar_t *argn[]=
	{
	    L"this", L"other"
	}
    ;
    
    mid_t mmid;
    anna_function_t *fun;

    anna_native_method_create(
	object_type,
	-1,
	L"__init__",
	0,
	&anna_object_init, 
	object_type, 1, argv, argn);
    
    mmid = anna_native_method_create(
	object_type,
	-1,
	L"__eq__Object__",
	0,
	&anna_object_eq, 
	object_type, 2, argv, argn);
    fun = anna_function_unwrap(*anna_static_member_addr_get_mid(object_type, mmid));
    anna_function_alias_add(fun, L"__eq__");
    
}
