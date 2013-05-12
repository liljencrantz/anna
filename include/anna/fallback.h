
#ifndef HAVE_WCSDUP
wchar_t *wcsdup(const wchar_t *in);
#endif

#ifndef HAVE_WCSCMP
int wcscmp(const wchar_t *s1, const wchar_t *s2);
#endif

#ifndef HAVE_WCSCHR
wchar_t *wcschr(const wchar_t *wcs, wchar_t wc);
#endif

#ifndef HAVE_WCSRCHR
wchar_t *wcsrchr(const wchar_t *wcs, wchar_t wc);
#endif

#ifndef HAVE_WCSLEN
size_t wcslen(const wchar_t *in);
#endif

#ifndef HAVE_FPUTWC
wint_t fputwc(wchar_t wc, FILE *stream);
wint_t putwc(wchar_t wc, FILE *stream);
#endif

#ifndef HAVE_FGETWC
wint_t fgetwc(FILE *stream);
wint_t getwc(FILE *stream);
#endif

#ifndef HAVE_WCSTOL
long wcstol(const wchar_t *nptr,
	    wchar_t **endptr,
	    int base);
#endif

#ifndef HAVE_PRCTL
static inline int prctl(
    int option, unsigned long arg2, unsigned long arg3,
    unsigned long arg4, unsigned long arg5);
#endif
