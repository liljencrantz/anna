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



void anna_char_type_create(anna_stack_frame_t *stack)
{
    anna_node_t *o_argv[] = 
	{
	    (anna_node_t *)anna_node_identifier_create(0, L"Char"),
	    (anna_node_t *)anna_node_identifier_create(0, L"Int"),
	}
    ;
    wchar_t *o_argn[] =
	{
	    L"this", L"ordinal"
	}
    ;

    char_type = anna_type_native_create(L"Char", stack);
    anna_node_call_t *definition = anna_type_definition_get(char_type);
    
    anna_member_add_node(
	definition, ANNA_MID_CHAR_PAYLOAD,  L"!charPayload", 
	0, (anna_node_t *)anna_node_identifier_create(0, L"Null") );
    
    anna_native_method_add_node(
	definition, -1, L"getOrdinal", 0, 
	(anna_native_t)&anna_char_i_get_ordinal, 
	(anna_node_t *)anna_node_identifier_create(0, L"Int"), 
	1, o_argv, o_argn);
    
    anna_native_method_add_node(
	definition, -1, L"setOrdinal", 0, 
	(anna_native_t)&anna_char_i_set_ordinal, 
	(anna_node_t *)anna_node_identifier_create(0, L"Int"), 
	2, o_argv, o_argn);
    
    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_property_create(
	    0,
	    L"ordinal",
	    (anna_node_t *)anna_node_identifier_create(0, L"Int") , 
	    L"getOrdinal",
	    L"setOrdinal"));

    anna_native_method_add_node(
	definition, -1, L"getUpper", 0, 
	(anna_native_t)&anna_char_i_get_upper, 
	(anna_node_t *)anna_node_identifier_create(0, L"Char"), 
	1, o_argv, o_argn);
    
    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_property_create(
	    0,
	    L"upper",
	    (anna_node_t *)anna_node_identifier_create(0, L"Char") , 
	    L"getUpper",
	    0));

    anna_native_method_add_node(
	definition, -1, L"getLower", 0, 
	(anna_native_t)&anna_char_i_get_lower, 
	(anna_node_t *)anna_node_identifier_create(0, L"Char"), 
	1, o_argv, o_argn);
    
    anna_node_call_add_child(
	definition,
	(anna_node_t *)anna_node_property_create(
	    0,
	    L"lower",
	    (anna_node_t *)anna_node_identifier_create(0, L"Char") , 
	    L"getLower",
	    0));

    anna_char_type_i_create(definition, stack);
}
