#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "util.h"
#include "anna.h"

static void anna_object_print_member(void *key_ptr,void *val_ptr, void *aux_ptr)
{
    wchar_t *key = (wchar_t *)key_ptr;
    anna_member_t *member = (anna_member_t *)val_ptr;
    //anna_object_t *obj = (anna_object_t *)aux_ptr;
    //anna_object_t *value = member->is_static?obj->type->static_member[member->offset]:obj->member[member->offset];    
    wprintf(
	L"  mid %d, name %ls: Type, %ls. %ls.\n", 
	anna_mid_get(key),
	key, 
	member->type?member->type->name:L"?",
	member->is_static?L"Static":L"Not static");
}

void anna_object_print(anna_object_t *obj)
{
    wprintf(L"%ls:\n", obj->type->name);
    hash_foreach2(&obj->type->name_identifier, &anna_object_print_member, obj);
}

anna_object_t *anna_object_create(anna_type_t *type) {
    assert(type);
    anna_object_t *result = 
	anna_object_create_raw(type->member_count);
    result->type = type;
    return result;
}

anna_object_t *anna_object_create_raw(size_t sz)
{
    anna_object_t *result = 
	calloc(
	    1,
	    sizeof(anna_object_t)+sizeof(anna_object_t *)*sz);
    int i;
    for(i=0; i<sz; i++)
    {
	result->member[i]=null_object;
    }
    
    return result;
}
