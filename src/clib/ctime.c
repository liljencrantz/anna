#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "common.h"
#include "util.h"
#include "wutil.h"
#include "anna.h"
#include "anna_module.h"
#include "anna_vm.h"
#include "anna_member.h"
#include "anna_mid.h"

#include "clib/lang/list.h"
#include "clib/clib.h"

ANNA_VM_NATIVE(anna_ctime_gettimeofday, 0)
{    
    struct timeval tv;
    int err = gettimeofday(&tv, 0);    
    
    if(err)
    {
	return null_entry;
    }
    
    anna_object_t *res = anna_list_create_imutable(int_type);
    anna_list_add(res, anna_from_int(tv.tv_sec));
    anna_list_add(res, anna_from_int(tv.tv_usec));
    return anna_from_obj(res);
}

ANNA_VM_NATIVE(anna_ctime_get_timezone, 1)
{
    return anna_from_int(timezone);    
}

ANNA_VM_NATIVE(anna_ctime_get_daylight, 1)
{
    return daylight ? anna_from_int(1):null_entry;    
}

void anna_ctime_load(anna_stack_template_t *stack)
{

    anna_module_function(
	stack, L"getTimeOfDay", 
	0, &anna_ctime_gettimeofday, 
	anna_list_type_get_imutable(int_type), 
	0, 0, 0,
	L"Returns the current time of day as the number of seconds and microseconds since the Epoch (midnight, January 1, 1970, UTC). Wrapper for the C gettimeofday function, see man 2 gettimeofday. Because the timezone argument of gettimeofday doesn't work, it's not included.");
    
    anna_type_t *type = anna_stack_wrap(stack)->type;
    anna_member_create_native_property(
	type, anna_mid_get(L"timezone"),
	int_type,
	&anna_ctime_get_timezone,
	0,
	L"The offset from UTC of the local timezone, in seconds");

    anna_member_create_native_property(
	type, anna_mid_get(L"daylight"),
	int_type,
	&anna_ctime_get_daylight,
	0,
	L"This property is non-null if at some part of the year, daylight saving time applies");
    
}

