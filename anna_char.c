#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_char.h"
#include "anna_type.h"

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

void anna_char_type_create(anna_stack_frame_t *stack)
{
    char_type = anna_type_native_create(L"Char", stack);
    anna_node_call_t *definition = anna_type_definition_get(float_type);
    
    anna_member_add_node(
	definition, ANNA_MID_CHAR_PAYLOAD,  L"!charPayload", 
	0, (anna_node_t *)anna_node_identifier_create(0, L"Null") );
    
    anna_char_type_i_create(definition, stack);
    anna_type_native_setup(char_type, stack);
}
