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

static anna_object_t *anna_char_i_get_ordinal(anna_object_t **param)
{
  return anna_int_create((int)anna_char_get(param[0]));
}

static anna_object_t *anna_char_i_set_ordinal(anna_object_t **param)
{
    if(param[1]==null_object)
	return null_object;
    int o = anna_int_get(param[1]);
    anna_char_set(param[0], (wchar_t)o);
    return param[1];
}

static anna_object_t *anna_char_i_get_upper(anna_object_t **param)
{
    return anna_char_create(towupper(anna_char_get(param[0])));
}

static anna_object_t *anna_char_i_get_lower(anna_object_t **param)
{
    return anna_char_create(towlower(anna_char_get(param[0])));
}



void anna_char_type_create(anna_stack_template_t *stack)
{
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

    anna_char_type_i_create(stack);
}
