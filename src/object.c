#include "anna/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna/fallback.h"
#include "anna/common.h"
#include "anna/util.h"
#include "anna/base.h"
#include "anna/alloc.h"
#include "anna/vm.h"
#include "anna/mid.h"
#include "anna/type.h"

void anna_object_print(anna_object_t *obj)
{
    anna_message(L"%ls:\n", obj->type->name);
    int i;
    for(i=0; i<anna_type_get_member_count(obj->type); i++)
    {
	anna_member_t *member = anna_type_get_member_idx(obj->type, i);
	anna_message(
	    L"  mid %d, name %ls: Type, %ls. %ls.\n", 
	    anna_mid_get(member->name),
	    member->name, 
	    member->type?member->type->name:L"?",
	    anna_member_is_static(member)?L"Static":L"Not static");	
    }
}

anna_object_t *anna_object_create(anna_type_t *type) {
     anna_object_t *result = 
	anna_object_create_raw(type->object_size);
    result->type = type;
    int i;

    for(i=0; i<type->member_count; i++)
    {
	result->member[i]=null_entry;
    }
//    anna_message(L"%ls\n", type->name);
    
    return result;
}

anna_object_t *anna_object_create_raw(size_t sz)
{
    anna_object_t *result = 
	anna_alloc_object(sz);
    return result;
}
