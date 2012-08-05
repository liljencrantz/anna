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
#include "anna/object.h"

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
