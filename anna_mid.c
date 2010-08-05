#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "util.h"
#include "anna.h"

static hash_table_t anna_mid_identifier;
static array_list_t anna_mid_identifier_reverse;
static size_t mid_pos = ANNA_MID_FIRST_UNRESERVED;

void anna_mid_init()
{
    al_init(&anna_mid_identifier_reverse);
    hash_init(
	&anna_mid_identifier,
	&hash_wcs_func, 
	&hash_wcs_cmp);
    
    anna_mid_put(L"!typeWrapperPayload", ANNA_MID_TYPE_WRAPPER_PAYLOAD);
    anna_mid_put(L"!callPayload", ANNA_MID_CALL_PAYLOAD);
    anna_mid_put(L"!stringPayload", ANNA_MID_STRING_PAYLOAD);
    anna_mid_put(L"!stringPayloadSize", ANNA_MID_STRING_PAYLOAD_SIZE);
    anna_mid_put(L"!charPayload", ANNA_MID_CHAR_PAYLOAD);
    anna_mid_put(L"!intPayload", ANNA_MID_INT_PAYLOAD);
    anna_mid_put(L"!listPayload", ANNA_MID_LIST_PAYLOAD);
    anna_mid_put(L"!listSize", ANNA_MID_LIST_SIZE);
    anna_mid_put(L"!listCapacity", ANNA_MID_LIST_CAPACITY);
    anna_mid_put(L"!functionTypePayload", ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
    anna_mid_put(L"!functionPayload", ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    anna_mid_put(L"!floatPayload", ANNA_MID_FLOAT_PAYLOAD);
    anna_mid_put(L"!functionStack", ANNA_MID_FUNCTION_WRAPPER_STACK);
    anna_mid_put(L"__call__", ANNA_MID_CALL_PAYLOAD);    
    anna_mid_put(L"__init__", ANNA_MID_INIT_PAYLOAD);
    anna_mid_put(L"!nodePayload", ANNA_MID_NODE_PAYLOAD);
    anna_mid_put(L"!memberPayload", ANNA_MID_MEMBER_PAYLOAD);
    anna_mid_put(L"!memberTypePayload", ANNA_MID_MEMBER_TYPE_PAYLOAD);
    anna_mid_put(L"!stackPayload", ANNA_MID_STACK_PAYLOAD);
    anna_mid_put(L"!stackTypePayload", ANNA_MID_STACK_TYPE_PAYLOAD);
    anna_mid_put(L"from", ANNA_MID_FROM);
    anna_mid_put(L"to", ANNA_MID_TO);
    anna_mid_put(L"step", ANNA_MID_STEP);
}



void anna_mid_put(wchar_t *name, size_t mid)
{
    size_t *offset_ptr = hash_get(&anna_mid_identifier, name);
    if(offset_ptr)
    {
	wprintf(L"Tried to reassign mid!\n");
	exit(1);
    }
   
    offset_ptr = malloc(sizeof(size_t));
    *offset_ptr = mid;
    hash_put(&anna_mid_identifier, name, offset_ptr);   
    al_set(&anna_mid_identifier_reverse, mid, wcsdup(name));
}

size_t anna_mid_get(wchar_t *name)
{
    size_t *offset_ptr = hash_get(&anna_mid_identifier, name);
    if(!offset_ptr)
    {      
	size_t gg = mid_pos++;
	anna_mid_put(name, gg);
	return gg;
    }
    return *offset_ptr;
}

wchar_t *anna_mid_get_reverse(size_t mid)
{
    return (wchar_t *)al_get(&anna_mid_identifier_reverse, mid);
}

