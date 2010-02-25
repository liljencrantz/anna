#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "duck.h"
#include "duck_node.h"
#include "duck_string.h"


duck_object_t *duck_string_create(size_t sz, wchar_t *data)
{
    duck_object_t *obj= duck_object_create(string_type);
    memcpy(duck_member_addr_get_mid(obj,DUCK_MID_STRING_PAYLOAD_SIZE), &sz, sizeof(size_t));
    wchar_t *res = malloc(sizeof(wchar_t) * sz);
    memcpy(res, data, sizeof(wchar_t) * sz);
    memcpy(duck_member_addr_get_mid(obj,DUCK_MID_STRING_PAYLOAD), &res, sizeof(wchar_t *));
    return obj;
}

wchar_t *duck_string_get_payload(duck_object_t *this)
{
    wchar_t *result;
    memcpy(&result, duck_member_addr_get_mid(this,DUCK_MID_STRING_PAYLOAD), sizeof(wchar_t *));
    return result;
}

size_t duck_string_get_payload_size(duck_object_t *this)
{
    size_t result;
    memcpy(&result, duck_member_addr_get_mid(this,DUCK_MID_STRING_PAYLOAD_SIZE), sizeof(size_t));
    return result;
}


void duck_string_type_create()
{
    string_type->member_count = 2;

    duck_member_create(string_type, DUCK_MID_STRING_PAYLOAD,  L"!stringPayload", 0, null_type);
    duck_member_create(string_type, DUCK_MID_STRING_PAYLOAD_SIZE,  L"!stringPayloadSize", 0, null_type);
}
