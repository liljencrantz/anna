/** \file intern.h

    Library for pooling common strings

*/

#ifndef ANNA_INTERN_H
#define ANNA_INTERN_H

#include <wchar.h>

/**
   Return an identical copy of the specified string from a pool of
   unique strings. If the string was not in the pool, create one.

   \param in the string to return an interned copy of
*/
wchar_t *anna_intern( wchar_t *in );

/**
   Return an identical copy of the specified string from a pool of
   unique strings. If an identical copy of the string was not in the
   pool, add this instance of this string to the pool, otherwise, call
   free on the original string and return the version from the pool.

   \param in the string to return an interned copy of
*/
wchar_t *anna_intern_or_free( wchar_t *in );

/**
   Return an identical copy of the specified string from a pool of
   unique strings. If an identical copy of the string was not in the
   pool, add this instance of this string to the pool. This call is
   suitable for string literals.

   \param in the string to add to the interned pool
*/
wchar_t *anna_intern_static( wchar_t *in );

#endif
