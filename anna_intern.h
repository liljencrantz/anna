/** \file intern.h

    Library for pooling common strings

*/

#ifndef ANNA_INTERN_H
#define ANNA_INTERN_H

#include <wchar.h>

/**
   Return an identical copy of the specified string from a pool of
   unique strings. If the string was not in the pool, add a copy.

   \param in the string to return an interned copy of
*/
wchar_t *anna_intern( wchar_t *in );

/**
   Return an identical copy of the specified string from a pool of
   unique strings. If the string was not in the pool, add this
   instance of this string to the pool, otherwise, call free on the
   original string and return the version from the pool.

   \param in the string to return an interned copy of
*/
wchar_t *anna_intern_or_free( wchar_t *in );

/**
   Insert the specified string literal into the pool of unique
   strings. The string will not first be copied, nor will it be
   free'd on exit.
   
   \param in the string to add to the interned pool
*/
wchar_t *anna_intern_static( wchar_t *in );

/**
   Free all interned strings. Only call this at shutdown.
*/
void anna_intern_free_all();

#endif
