#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_string.h"


anna_object_t *anna_string_create(size_t sz, wchar_t *data)
{
    anna_object_t *obj= anna_object_create(string_type);
    memcpy(anna_member_addr_get_mid(obj,ANNA_MID_STRING_PAYLOAD_SIZE), &sz, sizeof(size_t));
    wchar_t *res = malloc(sizeof(wchar_t) * sz);
    memcpy(res, data, sizeof(wchar_t) * sz);
    memcpy(anna_member_addr_get_mid(obj,ANNA_MID_STRING_PAYLOAD), &res, sizeof(wchar_t *));
    return obj;
}

wchar_t *anna_string_get_payload(anna_object_t *this)
{
    wchar_t *result;
    memcpy(&result, anna_member_addr_get_mid(this,ANNA_MID_STRING_PAYLOAD), sizeof(wchar_t *));
    return result;
}

size_t anna_string_get_payload_size(anna_object_t *this)
{
    size_t result;
    memcpy(&result, anna_member_addr_get_mid(this,ANNA_MID_STRING_PAYLOAD_SIZE), sizeof(size_t));
    return result;
}


void anna_string_type_create()
{
    anna_member_create(string_type, ANNA_MID_STRING_PAYLOAD,  L"!stringPayload", 0, null_type);
    anna_member_create(string_type, ANNA_MID_STRING_PAYLOAD_SIZE,  L"!stringPayloadSize", 0, null_type);
}
