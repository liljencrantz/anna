/**
   This file only contains fallback implementations of functions which
   have been found to be missing or broken by the configuration
   scripts.
*/

#include "anna/config.h"


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <wchar.h>
#include <wctype.h>
#include <string.h>
#include <dirent.h>
#include <stdarg.h>
#include <limits.h>
#include <assert.h>

#include "anna/fallback.h"

#ifndef HAVE_WCSDUP
wchar_t *wcsdup( const wchar_t *in )
{
	size_t len=wcslen(in);
	wchar_t *out = malloc( sizeof( wchar_t)*(len+1));
	if( out == 0 )
	{
		return 0;
	}

	memcpy( out, in, sizeof( wchar_t)*(len+1));
	return out;
	
}
#endif

#ifndef HAVE_WCSLEN
size_t wcslen(const wchar_t *in)
{
	const wchar_t *end=in;
	while( *end )
		end++;
	return end-in;
}
#endif

#ifndef HAVE_FPUTWC

wint_t fputwc(wchar_t wc, FILE *stream)
{
	int res;
	char s[MB_CUR_MAX+1];
	memset( s, 0, MB_CUR_MAX+1 );
	wctomb( s, wc );
	res = fputs( s, stream );
	return res==EOF?WEOF:wc;
}

wint_t putwc(wchar_t wc, FILE *stream)
{
	return fputwc( wc, stream );
}

#endif

#ifndef HAVE_FGETWC

wint_t fgetwc(FILE *stream)
{
	wchar_t res=0;
	mbstate_t state;
	memset (&state, '\0', sizeof (state));

	while(1)
	{
		int b = fgetc( stream );
		char bb;
			
		int sz;
			
		if( b == EOF )
			return WEOF;

		bb=b;
			
		sz = mbrtowc( &res, &bb, 1, &state );
			
		switch( sz )
		{
			case -1:
				memset (&state, '\0', sizeof (state));
				return WEOF;

			case -2:
				break;
			case 0:
				return 0;
			default:
				return res;
		}
	}

}


wint_t getwc(FILE *stream)
{
	return fgetwc( stream );
}


#endif

