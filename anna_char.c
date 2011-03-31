#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <wctype.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_char.h"
#include "anna_type.h"
#include "anna_int.h"
#include "anna_member.h"
#include "anna_function.h"
#include "anna_string.h"
#include "anna_vm.h"

#include "anna_char_i.c"

anna_object_t *anna_char_create(wchar_t value)
{
    
    anna_object_t *obj= anna_object_create(char_type);
    anna_char_set(obj, value);
    return obj;
}

void anna_char_set(anna_object_t *this, wchar_t value)
{
    memcpy(anna_member_addr_get_mid(this,ANNA_MID_CHAR_PAYLOAD), &value, sizeof(wchar_t));
}


wchar_t anna_char_get(anna_object_t *this)
{
    wchar_t result;
    memcpy(&result, anna_member_addr_get_mid(this,ANNA_MID_CHAR_PAYLOAD), sizeof(wchar_t));
    return result;
}

static inline anna_object_t *anna_char_i_get_ordinal_i(anna_object_t **param)
{
  return anna_int_create((int)anna_char_get(param[0]));
}
ANNA_VM_NATIVE(anna_char_i_get_ordinal, 1)

static inline anna_object_t *anna_char_i_set_ordinal_i(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    int o = anna_int_get(param[1]);
    anna_char_set(param[0], (wchar_t)o);
    return param[1];
}
ANNA_VM_NATIVE(anna_char_i_set_ordinal, 2)

static inline anna_object_t *anna_char_i_get_upper_i(anna_object_t **param)
{
    return anna_char_create(towupper(anna_char_get(param[0])));
}
ANNA_VM_NATIVE(anna_char_i_get_upper, 1)

static inline anna_object_t *anna_char_i_get_lower_i(anna_object_t **param)
{
    return anna_char_create(towlower(anna_char_get(param[0])));
}
ANNA_VM_NATIVE(anna_char_i_get_lower, 1)

static inline anna_object_t *anna_char_cmp_i(anna_object_t **param)
{
    if(unlikely(param[1]->type != char_type))
    {
	return null_object;
    }    
    anna_object_t *res =  anna_int_create(anna_char_get(param[0]) - anna_char_get(param[1]));
    return res;
}
ANNA_VM_NATIVE(anna_char_cmp, 2)

static inline anna_object_t *anna_char_to_string_i(anna_object_t **param)
{
    wchar_t ch = anna_char_get(param[0]);
    return anna_string_create(1, &ch);
}
ANNA_VM_NATIVE(anna_char_to_string, 1)

void anna_char_type_create(anna_stack_template_t *stack)
{
    anna_type_t *argv[] = 
	{
	    int_type, object_type
	}
    ;
    wchar_t *argn[]=
	{
	    L"this", L"other"
	}
    ;

    anna_member_create(
	char_type, 
	ANNA_MID_CHAR_PAYLOAD,  
	L"!charPayload", 
	0, null_type);

    anna_native_property_create(
	char_type,
	-1,
	L"ordinal",
	int_type,
	&anna_char_i_get_ordinal, 
	&anna_char_i_set_ordinal);
    
    anna_native_property_create(
	char_type,
	-1,
	L"upper",
	char_type,
	&anna_char_i_get_upper, 
	0);

    anna_native_property_create(
	char_type,
	-1,
	L"lower",
	char_type,
	&anna_char_i_get_lower, 
	0);

    anna_native_method_create(
	char_type,
	-1,
	L"__cmp__",
	0,
	&anna_char_cmp, 
	int_type,
	2, argv, argn);    
    
    anna_native_method_create(
	char_type,
	ANNA_MID_TO_STRING,
	L"toString",
	0,
	&anna_char_to_string, 
	string_type, 1, argv, argn);
    

    anna_char_type_i_create(stack);
}
