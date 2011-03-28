#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "util.h"
#include "anna.h"
#include "anna_type.h"
#include "anna_member.h"

static hash_table_t anna_mid_identifier;
static array_list_t anna_mid_identifier_reverse;
static mid_t mid_pos = ANNA_MID_FIRST_UNRESERVED;
static size_t anna_type_mid_max = 256;

static void anna_mid_free(void *key, void *val)
{
    free(key);
    free(val);
}


void anna_mid_destroy(void)
{
    al_destroy(&anna_mid_identifier_reverse);

    hash_foreach(&anna_mid_identifier, anna_mid_free);
    hash_destroy(&anna_mid_identifier);
}


size_t anna_mid_max_get()
{
    return anna_type_mid_max;
}

anna_member_t **anna_mid_identifier_create()
{
    return calloc(1,anna_type_mid_max*sizeof(anna_member_t *) );
}

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
    anna_mid_put(L"!listSpecialization", ANNA_MID_LIST_SPECIALIZATION);
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
    anna_mid_put(L"__eq__", ANNA_MID_EQ);
    anna_mid_put(L"!rangeFrom", ANNA_MID_RANGE_FROM);
    anna_mid_put(L"!rangeTo", ANNA_MID_RANGE_TO);
    anna_mid_put(L"!rangeStep", ANNA_MID_RANGE_STEP);
    anna_mid_put(L"!rangeOpen", ANNA_MID_RANGE_OPEN);
    anna_mid_put(L"__del__", ANNA_MID_DEL);
    anna_mid_put(L"!complexPayload", ANNA_MID_COMPLEX_PAYLOAD);
    anna_mid_put(L"hashCode", ANNA_MID_HASH_CODE);
    anna_mid_put(L"__cmp__", ANNA_MID_CMP);
    anna_mid_put(L"!hashPayload", ANNA_MID_HASH_PAYLOAD);
    anna_mid_put(L"!hashSpecialization1", ANNA_MID_HASH_SPECIALIZATION1);
    anna_mid_put(L"!hashSpecialization2", ANNA_MID_HASH_SPECIALIZATION2);
    anna_mid_put(L"toString", ANNA_MID_TO_STRING);
    anna_mid_put(L"!pairSpecialization1", ANNA_MID_PAIR_SPECIALIZATION1);
    anna_mid_put(L"!pairSpecialization2", ANNA_MID_PAIR_SPECIALIZATION2);
    anna_mid_put(L"!pairFirst", ANNA_MID_PAIR_FIRST);
    anna_mid_put(L"!pairSecond", ANNA_MID_PAIR_SECOND);
}



void anna_mid_put(wchar_t *name, mid_t mid)
{
    size_t *offset_ptr = hash_get(&anna_mid_identifier, name);
    if(offset_ptr)
    {
	wprintf(L"Tried to reassign mid!\n");
	exit(1);
    }
    
    offset_ptr = malloc(sizeof(size_t));
    *offset_ptr = mid;
    hash_put(&anna_mid_identifier, wcsdup(name), offset_ptr);   
    al_set(&anna_mid_identifier_reverse, mid, name);
}

size_t anna_mid_get_count()
{
    return anna_type_mid_max;
}


size_t anna_mid_get(wchar_t *name)
{
    size_t *offset_ptr = hash_get(&anna_mid_identifier, name);
    if(!offset_ptr)
    {      

	size_t gg = mid_pos++;
	if( mid_pos >= anna_type_mid_max)
	{
	    anna_type_mid_max += 128;
	    anna_type_reallocade_mid_lookup(anna_type_mid_max-128,anna_type_mid_max);
	}
	anna_mid_put(name, gg);
	return gg;
    }
    return *offset_ptr;
}

wchar_t *anna_mid_get_reverse(mid_t mid)
{
    return (wchar_t *)al_get(&anna_mid_identifier_reverse, mid);
}

