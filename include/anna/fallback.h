
#ifndef HAVE_WCSDUP

/**
   Create a duplicate string. Wide string version of strdup. Will
   automatically exit if out of memory.
*/
wchar_t *wcsdup(const wchar_t *in);

#endif

#ifndef HAVE_WCSLEN

/**
   Fallback for wcsen. Returns the length of the specified string.
*/
size_t wcslen(const wchar_t *in);

#endif
#ifndef HAVE_FPUTWC

/**
   Fallback implementation of fputwc
*/
wint_t fputwc(wchar_t wc, FILE *stream);
/**
   Fallback implementation of putwc
*/
wint_t putwc(wchar_t wc, FILE *stream);

#endif

#ifndef HAVE_FGETWC
/**
   Fallback implementation of fgetwc
*/
wint_t fgetwc(FILE *stream);

/**
   Fallback implementation of getwc
*/
wint_t getwc(FILE *stream);

#endif

#ifndef HAVE_WCSTOL

/**
   Fallback implementation. Convert a wide character string to a
   number in the specified base. This functions is the wide character
   string equivalent of strtol. For bases of 10 or lower, 0..9 are
   used to represent numbers. For bases below 36, a-z and A-Z are used
   to represent numbers higher than 9. Higher bases than 36 are not
   supported.
*/
long wcstol(const wchar_t *nptr,
	    wchar_t **endptr,
	    int base);

#endif

#ifndef HAVE_WCSTOL
#include <errno.h>

long convert_digit( wchar_t d, int base )
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

long wcstol(const wchar_t *nptr, 
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

#ifndef HAVE_PRCTL
static inline int prctl(
    int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5)
{
}
#endif
