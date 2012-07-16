/** \file common.c
	
Various functions, mostly string utilities, that are used by most
parts of fish.
*/
#include "anna/config.h"

#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include <stdio.h>

#include <wctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>		
#include <locale.h>
#include <execinfo.h>
#include <pthread.h>

#include "anna/fallback.h"
#include "anna/util.h"

#include "anna/wutil.h"
#include "anna/common.h"

int debug_level=3;

/**
   String buffer used by the wsetlocale function
*/
static string_buffer_t *setlocale_buff = 0;

void show_stackframe() 
{
	void *trace[32];
	char **messages = (char **)NULL;
	int i, trace_size = 0;

	trace_size = backtrace(trace, 32);
	messages = backtrace_symbols(trace, trace_size);

	if( messages )
	{
		debug( 0, L"Backtrace:" );
		for( i=0; i<trace_size; i++ )
		{
			anna_message( L"%s\n", messages[i]);
		}
		free( messages );
	}
}

wchar_t *str2wcs( const char *in )
{
    wchar_t *out;
    if(!in)
    {
	return 0;
    }
    size_t len = mbstowcs(0,in,0)+1;
    out = calloc(sizeof(wchar_t),(len));
    
    if( !out )
    {
	DIE_MEM();
    }
    if(mbstowcs(out, in, len) == (size_t)-1)
    {
	free(out);
	return 0;
    }
    anna_message(L"", wcslen(out));
    
/*    
    if(!str2wcs_internal( in, out ))
    {
	free(out);
	return(0);
	}*/
    return out;
	
}

wchar_t *str2wcs_internal( const char *in, wchar_t *out )
{
    size_t res=0;
    int in_pos=0;
    int out_pos = 0;
    mbstate_t state;
    size_t len;
    
    VERIFY( in, 0 );
    VERIFY( out, 0 );
    
    len = strlen(in);
    
    memset( &state, 0, sizeof(state) );
    
    while( in[in_pos] )
    {
	res = mbrtowc( &out[out_pos], &in[in_pos], len-in_pos, &state );
	
	switch( res )
	{
	    case (size_t)(-2):
	    case (size_t)(-1):
	    {
		free(out);
		return 0;
	    }
	    
	    case 0:
	    {
		out[out_pos] = 0;
		anna_message(L"", wcslen(out));
		return out;
	    }
	    
	    default:
	    {
		in_pos += res;
		break;
	    }
	}
	out_pos++;
    }
    out[out_pos] = 0;
    anna_message(L"", wcslen(out));
    return out;	
}

char *wcs2str( const wchar_t *in )
{
    char *out;	
	
    out = malloc( MAX_UTF8_BYTES*wcslen(in)+1 );

    if( !out )
    {
	DIE_MEM();
    }

    return wcs2str_internal( in, out );
}

char *wcs2str_internal( const wchar_t *in, char *out )
{
	size_t res=0;
	int in_pos=0;
	int out_pos = 0;
	mbstate_t state;

	VERIFY( in, 0 );
	VERIFY( out, 0 );
	
	memset( &state, 0, sizeof(state) );
	
	while( in[in_pos] )
	{
	    res = wcrtomb( &out[out_pos], in[in_pos], &state );
	    
	    if( res == (size_t)(-1) )
	    {
		debug( 1, L"Wide character %d has no narrow representation", in[in_pos] );
		memset( &state, 0, sizeof(state) );
	    }
	    else
	    {
		out_pos += res;
	    }
	    in_pos++;
	}
	out[out_pos] = 0;
	
	return out;	
}

char **wcsv2strv( const wchar_t **in )
{
	int count =0;
	int i;

	while( in[count] != 0 )
		count++;
	char **res = malloc( sizeof( char *)*(count+1));
	if( res == 0 )
	{
		DIE_MEM();		
	}

	for( i=0; i<count; i++ )
	{
		res[i]=wcs2str(in[i]);
	}
	res[count]=0;
	return res;

}

wchar_t **strv2wcsv( const char **in )
{
	int count =0;
	int i;

	while( in[count] != 0 )
		count++;
	wchar_t **res = malloc( sizeof( wchar_t *)*(count+1));
	if( res == 0 )
	{
		DIE_MEM();
	}

	for( i=0; i<count; i++ )
	{
		res[i]=str2wcs(in[i]);
	}
	res[count]=0;
	return res;

}

const wchar_t *wsetlocale(int category, const wchar_t *locale)
{
    int do_free = 0;
    char *lang = 0;
    if(locale)
    {	
	if(wcslen(locale) == 0)
	{
	    lang = "";
	}
	else
	{
	    char *lang = wcs2str(locale);
	    do_free = 1;
	}
    }
    
    char *res = setlocale(category,lang);
    
    if(do_free)
	free( lang );

    if(!res)
	return 0;
    
    if(!setlocale_buff)
    {
	setlocale_buff = sb_new();
    }
    
    sb_clear( setlocale_buff );
    sb_printf( setlocale_buff, L"%s", res );
    
    return sb_content(setlocale_buff);
}

void debug( int level, const wchar_t *msg, ... )
{
       
	va_list va;

	string_buffer_t sb;

	int errno_old = errno;
	wchar_t *level_name[] = 
	    {
		L"Spam", L"Info", L"Warning", L"Error",
	    }
	;
	
	wchar_t *pre = L"CRITICAL";
	if( level < debug_level )
		return;

	if(level >= 0 && level <= D_ERROR)
	{
	    pre = level_name[level];
	}
		
	VERIFY( msg, );
		
	sb_init( &sb );

	sb_printf( &sb, L"%ls: ", pre);

	va_start( va, msg );	
	sb_vprintf( &sb, msg, va );
	va_end( va );	

	anna_print( 2, sb_content(&sb), -1 );	

	sb_destroy( &sb );	

	errno = errno_old;
}

static string_buffer_t *anna_message_buffer = 0;
static pthread_mutex_t anna_common_message_mutex = PTHREAD_MUTEX_INITIALIZER;

void anna_message_set_buffer(string_buffer_t *message_buffer)
{
    anna_message_buffer = message_buffer;
}

void anna_message(const wchar_t *msg, ... )
{
    int errno_old = errno;
    string_buffer_t sb_fallback;

    string_buffer_t *buff = anna_message_buffer;

    pthread_mutex_lock(&anna_common_message_mutex);

    if(!buff)
    {
	buff = &sb_fallback;
	sb_init( buff );
    }
        
    va_list va;
    
    VERIFY( msg, );
    
    va_start( va, msg );	
    sb_vprintf( buff, msg, va );
    va_end( va );	
    
    pthread_mutex_unlock(&anna_common_message_mutex);
    
    if(!anna_message_buffer)
    {
	anna_print( 2, sb_content(buff), -1 );
	sb_destroy( buff );	
    }
    
    errno = errno_old;
}


static int anna_print_convert(wchar_t *src, size_t src_sz, char **dst, size_t *dst_sz)
{
    size_t i;
    static char *res=0;
    static size_t res_sz;
    
    size_t max_sz = (src_sz+2) * MB_LEN_MAX;
    if(res_sz < max_sz)
    {
	res_sz = max_sz;
	res = realloc(res, res_sz);
    }

    char *ptr = res;
    for(i=0; i<src_sz; i++)
    {
	int steps = wctomb(ptr, src[i]);
	if(steps == -1)
	{
	    return -1;
	}
	ptr += steps;
    }
    *dst = res;
    *dst_sz = ptr-res;
    return 0;
}

static pthread_mutex_t anna_common_print_mutex = PTHREAD_MUTEX_INITIALIZER;

void anna_print(int fd, wchar_t *str, ssize_t slen)
{
    if(slen == -1)
    {
	slen = wcslen(str);
    }
    
    char *narrow;
    size_t len;

    pthread_mutex_lock(&anna_common_print_mutex);

    if(anna_print_convert(str, slen, &narrow, &len))
    {
	narrow = "Failed to convert wide character string to narrow character string for printing.\n";
	len = strlen(narrow);
    }
    while(1)
    {
	int res = write(fd, narrow, len);    
	if(res != -1)
	{
	    // Ok
	    break;
	}
	
	// Error!
	if(errno != EAGAIN)
	{
	    /* 
	       Output error. Exit. Little point in writing an error
	       message, since, that's basically exactly what this
	       function is used for, and it just failed. So we die,
	       quitely. Alone.
	    */
	    exit(1);
	}	
    }    
    pthread_mutex_unlock(&anna_common_print_mutex);
}
