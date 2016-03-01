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

#ifndef HAVE_WCSCMP
int wcscmp(const wchar_t *s1, const wchar_t *s2)
{
    while(1)
    {
	if(!*s1)
	{
	    return *s2 ? 1 : 0;
	}
	if(!*s2)
	{
	    return -1;
	}
	int cmp = *s1 - *s2;
	if(cmp != 0)
	    return cmp;
	s1++;
	s2++;
    }
    return 0;
}
#endif

#ifndef HAVE_WCSCHR
wchar_t *wcschr(const wchar_t *wcs, wchar_t wc)
{
    while(*wcs)
    {
	if (*wcs == wc)
	    return wcs;
	wcs++;
    }
    return 0;
}
#endif

#ifndef HAVE_WCSRCHR
wchar_t *wcsrchr(const wchar_t *wcs, wchar_t wc)
{
    wchar_t *res = 0;
    while(*wcs)
    {
	if (*wcs == wc)
	    res = wcs;
	wcs++;
    }
    return res;
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

#ifndef HAVE_WCSTOL
#include <errno.h>

static long convert_digit( wchar_t d, int base )
{
    long res=-1;
    if( (d <= L'9') && (d >= L'0') )
    {
	res = d - L'0';
    }
    else if( (d <= L'z') && (d >= L'a') )
    {
	res = d + 10 - L'a';		
    }
    else if( (d <= L'Z') && (d >= L'A') )
    {
	res = d + 10 - L'A';		
    }
    if( res >= base )
    {
	res = -1;
    }	
    return res;
}

long wcstol(
    const wchar_t *nptr, 
    wchar_t **endptr,
    int base)
{
    long long res=0;
    int is_set=0;
    if( base > 36 )
    {
	errno = EINVAL;
	return 0;
    }
    
    while( 1 )
    {
	long nxt = convert_digit( *nptr, base );
	if( endptr != 0 )
	    *endptr = (wchar_t *)nptr;
	if( nxt < 0 )
	{
	    if( !is_set )
	    {
		errno = EINVAL;
	    }
	    return res;			
	}
	res = (res*base)+nxt;
	is_set = 1;
	if( res > LONG_MAX )
	{
	    errno = ERANGE;
	    return LONG_MAX;
	}
	if( res < LONG_MIN )
	{
	    errno = ERANGE;
	    return LONG_MIN;
	}
	nptr++;
    }
}
#endif

#if HAVE_DECL_CPOW == 0
#include <math.h>

double complex cpow(
    double complex a,
    double complex z)
{
    double x = creal (z);
    double y = cimag (z);
    double absa = cabs (a);
    if (absa == 0.0) {
        return (0.0 + 0.0 * I);
    }
    double arga = carg (a);
    double r = pow (absa, x);
    double theta = x * arga;
    if (y != 0.0) {
        r = r * exp (-y * arga);
        theta = theta + y * log (absa);
    }

    return r * cos (theta) + (r * sin (theta)) * I;
}
#endif

#ifndef HAVE_CLEARENV
char *environ[] = {NULL};

int clearenv(void)
{
    return 1;
}
#endif

#if HAVE_DECL_FDATASYNC == 0
int fdatasync(int fd)
{
    return fsync(fd);
}
#endif

#if HAVE_DECL_CLOG == 0
#include <complex.h>

double complex clog(double complex z)
{
    double rr = cabs(z);
    double p = log(rr);
    rr = atan2(cimag(z), creal(z));
    return p + rr * I;
}
#endif

#if HAVE_DECL_CLOG10 == 0
#include <complex.h>

double complex clog10(double complex z)
{
    return 0;
}
#endif
