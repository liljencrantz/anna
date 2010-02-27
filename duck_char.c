#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "duck.h"
#include "duck_node.h"
#include "duck_char.h"



duck_object_t *duck_char_create(wchar_t value)
{
    duck_object_t *obj= duck_object_create(char_type);
    duck_char_set(obj, value);
    return obj;
}

void duck_char_set(duck_object_t *this, wchar_t value)
{
    memcpy(duck_member_addr_get_mid(this,DUCK_MID_CHAR_PAYLOAD), &value, sizeof(wchar_t));
}


wchar_t duck_char_get(duck_object_t *this)
{
    wchar_t result;
    memcpy(&result, duck_member_addr_get_mid(this,DUCK_MID_CHAR_PAYLOAD), sizeof(wchar_t));
    return result;
}

void duck_char_type_create()
{
    duck_member_create(char_type, DUCK_MID_CHAR_PAYLOAD,  L"!charPayload", 0, null_type);
}
