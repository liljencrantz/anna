#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "anna_string_internal.h"
#include "anna.h"

#define LAPS 16

static long long get_time()
{
    struct timeval time_struct;
    gettimeofday( &time_struct, 0 );
    return 1000000ll*time_struct.tv_sec+time_struct.tv_usec;
}

static void anna_string_append_test(size_t min_len, size_t max_len, wchar_t *msg)
{
    long long start_time = get_time();
    int j=0;
    size_t stop = 5000000;
    
    for(j=0;j<LAPS;j++)
    {
	int i;
	anna_string_t a, b, c, d, e;
	
	size_t chars_done=0;
	
	asi_init(&a);
	asi_init(&b);
	asi_init(&c);
	asi_init(&d);
	asi_init(&e);
	asi_init_from_ptr(&e, L"abcdefghijklmnopqrstuvwxyz0123456789ABCD", 40);
	for(i=0; i<= max_len/20; i++)
	{
	    asi_append(&d, &e, 0, 40);
	}
      
	while(1)
	{
	    size_t len = min_len + rand()%(max_len-min_len);
	    size_t offset = rand()%max_len;
	    anna_string_t *dest=0;
	    switch(rand()%3)
	    {
		case 0:
		    dest = &a;
		    break;
		case 1:
		    dest = &b;
		    break;
		case 2:
		    dest = &c;
		    break;
	    }
	    asi_append(dest, &d, offset, len);
	    chars_done += len;
	  
	    if(chars_done > stop) 
	    {
		break;
	    }
	}
    }
  
    long long stop_time = get_time();
  
    wprintf(L"Appended %ls @ %f million chars/s\n",
	    msg, 1.0*((double)stop*LAPS)/(stop_time-start_time));
}

static void anna_string_append_test2(size_t min_len, size_t max_len, wchar_t *msg)
{
    long long start_time = get_time();
    int j=0;
    size_t stop = 100000;
    
    for(j=0;j<LAPS;j++)
    {
	int i;
	anna_string_t a, b, c, d, e;
	
	size_t chars_done=0;
	
	asi_init(&a);
	asi_init(&b);
	asi_init(&c);
	asi_init(&d);
	asi_init(&e);
	asi_init_from_ptr(&e, L"abcdefghijklmnopqrstuvwxyz0123456789ABCD", 40);
	for(i=0; i<= max_len/20; i++)
	{
	    asi_append(&d, &e, 0, 40);
	}
      
	while(1)
	{
	    size_t len = min_len + rand()%(max_len-min_len);
	    size_t offset = rand()%max_len;
	    anna_string_t *dest=0;
	    switch(rand()%3)
	    {
		case 0:
		    dest = &a;
		    break;
		case 1:
		    dest = &b;
		    break;
		case 2:
		    dest = &c;
		    break;
	    }
	    
	    size_t d_o = asi_get_count(dest)?rand()%asi_get_count(dest):0;
	    size_t d_l = 0;
	    asi_replace(dest, &d, d_o, d_l, offset, len);
	    chars_done += len;
	  
	    if(chars_done > stop) 
	    {
		break;
	    }
	}
    }
  
    long long stop_time = get_time();
  
    wprintf(L"Inserted %ls @ %f million chars/s\n",
	    msg, 1.0*((double)stop*LAPS)/(stop_time-start_time));
}

int main()
{

    anna_string_append_test(1, 16, L"short strings (1-16 chars)");  
    anna_string_append_test(1, 800, L"mixed strings (1-800 chars)");  
    anna_string_append_test(400, 800, L"long strings (400-800 chars)");  

    anna_string_append_test2(1, 16, L"short strings (1-16 chars)");  
    anna_string_append_test2(1, 800, L"mixed strings (1-800 chars)");  
    anna_string_append_test2(400, 800, L"long strings (400-800 chars)");  
    return 0;
}
