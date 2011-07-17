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
#include "anna_member.h"
#include "anna_function.h"
#include "anna_vm.h"
#include "anna_mid.h"

#include "clib/anna_int.h"
#include "clib/anna_string.h"

#include "autogen/anna_char_i.c"

anna_object_t *anna_char_create(wchar_t value)
{
    anna_object_t *obj= anna_object_create(char_type);
    anna_char_set(obj, value);
    return obj;
}

void anna_char_set(anna_object_t *this, wchar_t value)
{
    memcpy(anna_entry_get_addr(this,ANNA_MID_CHAR_PAYLOAD), &value, sizeof(wchar_t));
}


wchar_t anna_char_get(anna_object_t *this)
{
    wchar_t result;
    memcpy(&result, anna_entry_get_addr(this,ANNA_MID_CHAR_PAYLOAD), sizeof(wchar_t));
    return result;
}

ANNA_VM_NATIVE(anna_char_i_get_ordinal, 1)
{
    return anna_from_int((int)anna_as_char(param[0]));
}

ANNA_VM_NATIVE(anna_char_i_get_upper, 1)
{
    return anna_from_char(towupper(anna_as_char(param[0])));
}

ANNA_VM_NATIVE(anna_char_i_get_lower, 1)
{
    return anna_from_char(towlower(anna_as_char(param[0])));
}

ANNA_VM_NATIVE(anna_char_cmp, 2)
{
    if(unlikely(anna_is_obj(param[1]) && anna_as_obj(param[1])->type != char_type))
    {
	return null_entry;
    }
    return anna_from_int(anna_as_char(param[0]) - anna_as_char(param[1]));
}

ANNA_VM_NATIVE(anna_char_to_string, 1)
{
    wchar_t ch = anna_as_char(param[0]);
    return anna_from_obj(anna_string_create(1, &ch));
}

void anna_char_type_create()
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

    anna_member_create(char_type, ANNA_MID_CHAR_PAYLOAD, 0, null_type);

    anna_member_create_native_property(
	char_type, anna_mid_get(L"ordinal"),
	int_type, &anna_char_i_get_ordinal, 0,
	L"The ordinal number of this Char");
    
    anna_member_create_native_property(
	char_type,
	anna_mid_get(L"upper"),
	char_type,
	&anna_char_i_get_upper,
	0,
	L"The upper case equivalent of this Char");

    anna_member_create_native_property(
	char_type,
	anna_mid_get(L"lower"),
	char_type,
	&anna_char_i_get_lower,
	0,
	L"The lower case eqivalent of this Char");
    
    anna_member_create_native_method(
	char_type, anna_mid_get(L"__cmp__"), 0,
	&anna_char_cmp, int_type, 2, argv, argn);    
    
    anna_member_create_native_method(
	char_type, ANNA_MID_TO_STRING, 0,
	&anna_char_to_string, string_type, 1,
	argv, argn);

    anna_char_type_i_create();
}