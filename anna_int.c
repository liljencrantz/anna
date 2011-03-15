#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_int.h"
#include "anna_type.h"
#include "anna_member.h"
#include "anna_function.h"

#include "anna_int_i.c"

anna_object_t *anna_int_create(int value)
{
  anna_object_t *obj= anna_object_create(int_type);
  anna_int_set(obj, value);
  return obj;
}

void anna_int_set(anna_object_t *this, int value)
{
    memcpy(anna_member_addr_get_mid(this,ANNA_MID_INT_PAYLOAD), &value, sizeof(int));
}

int anna_int_get(anna_object_t *this)
{
    int result;
    memcpy(&result, anna_member_addr_get_mid(this,ANNA_MID_INT_PAYLOAD), sizeof(int));
    return result;
}

static anna_object_t *anna_int_init(anna_object_t **param)
{
    anna_int_set(param[0], 0);
    return param[0];
}

void anna_int_type_create(anna_stack_template_t *stack)
{
    anna_type_t *i_argv[] = 
	{
	    int_type
	}
    ;
    wchar_t *i_argn[]=
	{
	    L"this"
	}
    ;

    anna_member_create(
	int_type,
	ANNA_MID_INT_PAYLOAD, 
	L"!intPayload", 
	0,
	null_type);

    anna_native_method_create(
	int_type,
	-1,
	L"__init__",
	0,
	&anna_int_init, 
	object_type,
	1, i_argv, i_argn);    

    
    anna_int_type_i_create(stack);
}
