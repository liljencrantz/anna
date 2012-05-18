#include "anna/config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "anna/fallback.h"
#include "anna/common.h"
#include "anna/util.h"
#include "anna/wutil.h"
#include "anna/base.h"
#include "anna/module.h"
#include "anna/module_data.h"
#include "anna/vm.h"
#include "anna/member.h"
#include "anna/mid.h"
#include "anna/type.h"
#include "anna/intern.h"

#include "anna/lib/lang/list.h"
#include "anna/lib/lang/string.h"
#include "anna/lib/clib.h"

#define CTIME_BUFF_SZ 512

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

ANNA_VM_NATIVE(anna_ctime_get_timezone_name, 1)
{
    char *tz = getenv("TZ");
    if(tz)
    {
	wchar_t *wtz = str2wcs(tz);
	anna_object_t *res = anna_string_create(wcslen(wtz), wtz);
	free(wtz);
	return anna_from_obj(res);
    }

    FILE *fp = fopen("/etc/timezone", "r");
    char buff[CTIME_BUFF_SZ];
    if(fp)
    {
	char *res = fgets(buff, CTIME_BUFF_SZ, fp);
	fclose(fp);
	if(res)
	{
	    /* Strip newline and anything following it */
	    char *ptr;
	    for(ptr=&buff[0]; *ptr; ptr++)
	    {
		if(*ptr == '\n')
		{
		    *ptr=0;
		    break;
		}
	    }
	    
	    wchar_t *wtz = str2wcs(buff);
	    anna_object_t *res = anna_string_create(wcslen(wtz), wtz);
	    free(wtz);
	    return anna_from_obj(res);	    
	}
    }
    
    return null_entry;
}

ANNA_VM_NATIVE(anna_ctime_set_timezone_name, 2)
{
    if(param[1] == null_entry)
    {
	unsetenv("TZ");
    }
    else
    {
	wchar_t *wtz = anna_string_payload(anna_as_obj(param[1]));
	char *ntz = wcs2str(wtz);
	free(wtz);
	
	setenv("TZ", ntz, 1);
	free(ntz);
    }
    tzset();
    return param[1];
}

ANNA_VM_NATIVE(anna_ctime_mktime, 8)
{
    struct tm tm = 
	{
	    anna_as_int(param[0]),
	    anna_as_int(param[1]),
	    anna_as_int(param[2]),
	    anna_as_int(param[3]),
	    anna_as_int(param[4]),
	    anna_as_int(param[5]),
	    0,
	    0,
	    param[6] == null_entry ? -1: anna_as_int(param[6])
	}
    ;
    
    int reset_tz=0;
    char *tz=0;
    char *ntz=0;
    
    if(param[7] != null_entry)
    {
	reset_tz=1;
	wchar_t *wtz = anna_string_payload(anna_as_obj(param[7]));
	ntz = wcs2str(wtz);
	free(wtz);

	tz = getenv("TZ");
	setenv("TZ", ntz, 1);
	tzset();
    }
    
    anna_entry_t *res = anna_from_int(mktime(&tm));

    if(reset_tz)
    {
	if (tz)
	    setenv("TZ", tz, 1);
	else
	    unsetenv("TZ");
	tzset();
	free(ntz);
    }
    return res;
}

static anna_entry_t *handle_tm(struct tm *tm)
{
    anna_object_t *res = anna_list_create_imutable(int_type);
    anna_list_add(res, anna_from_int(tm->tm_sec));
    anna_list_add(res, anna_from_int(tm->tm_min));
    anna_list_add(res, anna_from_int(tm->tm_hour));
    anna_list_add(res, anna_from_int(tm->tm_mday));
    anna_list_add(res, anna_from_int(tm->tm_mon));
    anna_list_add(res, anna_from_int(tm->tm_year));
    anna_list_add(res, anna_from_int(tm->tm_wday));
    anna_list_add(res, anna_from_int(tm->tm_yday));
    anna_list_add(res, tm->tm_isdst?anna_from_int(1):null_entry);
    return anna_from_obj(res);
}

ANNA_VM_NATIVE(anna_ctime_break_time, 2)
{
    time_t timestamp;
    struct tm tm;

    if(param[0] == null_entry)
    {
	return null_entry;
    }
    
    timestamp = anna_as_int(param[0]);
    if(param[1] == null_entry)
    {
	if(!localtime_r(&timestamp, &tm))
	{
	    return null_entry;
	}
	
	return handle_tm(&tm);
    }
    
    wchar_t *wtz = anna_string_payload(anna_as_obj(param[1]));
    if(wcscmp(wtz, L"UTC")==0)
    {
	if(!gmtime_r(&timestamp, &tm))
	{
	    return null_entry;
	}
	return handle_tm(&tm);
    }
    
    char *ntz = wcs2str(wtz);
    free(wtz);
    
    char *tz = getenv("TZ");
    setenv("TZ", ntz, 1);
    tzset();
    
    struct tm *res = localtime_r(&timestamp, &tm);    

    if (tz)
	setenv("TZ", tz, 1);
    else
	unsetenv("TZ");
    tzset();
    free(ntz);
    
    return res ? handle_tm(&tm) : null_entry;
}

static void anna_ctime_broken_load(anna_stack_template_t *stack)
{
    anna_stack_document(
	stack,
	L"The constants found in this module are used to access the different fields of a broken down time as returned by ctime.breakTime.");
    
    anna_module_const_int(stack, L"second", 0, L"Number of seconds past last whole minute.");
    anna_module_const_int(stack, L"minute", 1, L"Number of minutes past last whole hour.");
    anna_module_const_int(stack, L"hour", 2, L"Number of hours past midnight.");
    anna_module_const_int(stack, L"dayOfMonth", 3, L"Number of days since first day of month.");
    anna_module_const_int(stack, L"month", 4, L"Month, January is 0.");
    anna_module_const_int(stack, L"year", 5, L"Number of years since 1900.");
    anna_module_const_int(stack, L"dayOfWeek", 6, L"Day of week, Sunday is 0.");
    anna_module_const_int(stack, L"dayOfYear", 7, L"Number of days since January 1:st.");
    anna_module_const_int(stack, L"daylightSaving?", 8, L"If non-null, daylight savings time is in effect");
}

void anna_ctime_load(anna_stack_template_t *stack)
{
    anna_stack_document(
	stack,
	L"A simple, low level time API. Unlike most other c* api:s in "
	L"Anna, this one doesn't closely mimic the underlying C "
	L"functions.");

    anna_stack_document(
	stack,
	L"Specifically, it is rather different in how timezones "
	L"are handled, as a timezone name can be passed as an argument to "
	L"various functions.");
    
    anna_stack_document(
	stack,
	L"Internally, timezone handling is handled using an extremely ugly "
	L"hack: The TZ environment variable is set, and the tzset function "
	L"is called. This temporarily changes the timezone information of "
	L"the running application, something which is obviously not thread "
	L"safe, reliable or good practice. Unfortunatly, this seems to be the "
	L"only reliable way to perform timezone calculations using the "
	L"standard C API.");
      
    anna_stack_document(
	stack,
	L"This library deals with time in two different formats, time "
	L"stamps and broken down time.");
    
      
    anna_stack_document(
	stack,
	L"A time stamp counts the number of whole seconds that have passed "
	L"since the Epoch (midnight, January 1, 1970, UTC). A time stamp "
	L"is never relative to any timezone other than UTC. The "
	L"getTimeOfDay function returns a time stamp plus an additional "
	L"microsecond fraction of time.");
    
    anna_stack_document(
	stack,
	L"Broken down time a time separated into fields representing the "
	L"number of seconds past the minute (0..60), minutes past the hour "
	L"(0..59), hours after midnight (0..24), day of the month (1..31), "
	L"months since january (0..11), years after 1900, days since "
	L"sunday (0..6), days since january 1 (0..365) and finally a field "
	L"that is set to 1 if daylight saving time is in effect, 0 "
	L"otherwise. Broken down time is always relative to some specific "
	L"time zone.");

    anna_module_data_t modules[] = 
	{
	    { L"broken", 0, anna_ctime_broken_load },
	};

    anna_module_data_create(modules, stack);
    
    anna_module_function(
	stack, L"getTimeOfDay", 
	0, &anna_ctime_gettimeofday, 
	anna_list_type_get_imutable(int_type), 
	0, 0, 0, 0,
	L"Returns the current time of day as the number of seconds and microseconds since the Epoch (midnight, January 1, 1970, UTC). Wrapper for the C gettimeofday function, see man 2 gettimeofday. Because the timezone argument of gettimeofday doesn't reliably work, it's not supported. Use the timezoneName and timezoneOffset properties for working timezone information.");
    
    anna_type_t *mt_argv[] = 
	{
	    int_type, int_type, int_type, 
	    int_type, int_type, int_type, 
	    int_type, string_type
	}
    ;
    wchar_t *mt_argn[] = 
	{
	    L"sec", L"min", L"hour", 
	    L"mday", L"month", L"year", 
	    L"daylightSaving?", L"timezone"
	}
    ;

    anna_module_function(
	stack, L"mkTime", 
	0, &anna_ctime_mktime, 
	int_type, 
	8, mt_argv, mt_argn, 0,
	L"Convert a broken down time to a time stamp");
    
    anna_type_t *a_argv[] = 
	{
	    int_type, string_type
	}
    ;
    wchar_t *a_argn[] = 
	{
	    L"timestamp", L"timezone"
	}
    ;
    
    anna_module_function(
	stack, L"breakTime", 
	0, &anna_ctime_break_time, 
	anna_list_type_get_imutable(int_type), 
	2, a_argv, a_argn, 0,
	L"Convert a time stamp to broken down time in the specified timezone. If no timezone is specified, this function is equivalent to the localtime C function.");
    
    anna_type_t *type = anna_stack_wrap(stack)->type;

    anna_member_create_native_property(
	type, anna_mid_get(L"timezoneOffset"),
	int_type,
	&anna_ctime_get_timezone,
	0,
	L"The offset from UTC of the local timezone, in seconds.");

    anna_member_create_native_property(
	type, anna_mid_get(L"timezoneName"),
	string_type, 
	&anna_ctime_get_timezone_name,
	&anna_ctime_set_timezone_name,
	L"The name of the currently configured timezone.");
}
