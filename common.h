/** \file common.h
	Prototypes for various functions, mostly string utilities, that are used by most parts of fish.
*/

#ifndef ANNA_COMMON_H
/**
   Header guard
*/
#define ANNA_COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#include "util.h"

/**
   Maximum number of bytes used by a single utf-8 character
*/
#define MAX_UTF8_BYTES 6

/**
   This is in the unicode private use area.
*/
#define ENCODE_DIRECT_BASE 0xf100

/**
  Highest legal ascii value
*/
#define ASCII_MAX 127u

/**
  Highest legal 16-bit unicode value
*/
#define UCS2_MAX 0xffffu

/**
  Highest legal byte value
*/
#define BYTE_MAX 0xffu

/**
  Escape special fish syntax characters like the semicolon
 */
#define UNESCAPE_SPECIAL 1

/**
  Allow incomplete escape sequences
 */
#define UNESCAPE_INCOMPLETE 2

/**
   Escape all characters, including magic characters like the semicolon
 */
#define ESCAPE_ALL 1
/**
   Do not try to use 'simplified' quoted escapes, and do not use empty quotes as the empty string
 */
#define ESCAPE_NO_QUOTED 2

#ifdef unused
#elif defined(__GNUC__)
# define unused(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define unused(x) /*@unused@*/ x
#else
# define unused(x) x
#endif

/**
   The verbosity level of fish. If a call to debug has a severity
   level higher than \c debug_level, it will not be printed.
*/
extern int debug_level;

/**
   Name of the current program. Should be set at startup. Used by the
   debug function.
*/
extern wchar_t *program_name;

/**
   This macro is used to check that an input argument is not null. It
   is a bit lika a non-fatal form of assert. Instead of exit-ing on
   failiure, the current function is ended at once. The second
   parameter is the return value of the current function on failiure.
*/
#define VERIFY( arg, retval )											\
	if( !(arg) )														\
	{																	\
		debug( 0,														\
			   _( L"function %s called with null value for argument %s. " ), \
			   __func__,												\
			   #arg );													\
		bugreport();													\
		show_stackframe();												\
		return retval;													\
	}

/**
   Pause for input, then exit the program. If supported, print a backtrace first.
*/
#define FATAL_EXIT()											\
	{															\
		int exit_read_count;char exit_read_buff;				\
		show_stackframe();										\
		exit_read_count=read( 0, &exit_read_buff, 1 );			\
		exit( 1 );												\
	}															\
		

/**
   Exit program at once, leaving an error message about running out of memory.
*/
#define DIE_MEM()														\
	{																	\
		fwprintf( stderr,												\
				  L"fish: Out of memory on line %d of file %s, shutting down fish\n", \
				  __LINE__,												\
				  __FILE__ );											\
		FATAL_EXIT();														\
	}

/**
   Shorthand for wgettext call
*/
#define _(wstr) wstr

/**
   Noop, used to tell xgettext that a string should be translated,
   even though it is not directly sent to wgettext. 
*/
#define N_(wstr) wstr

/**
   Check if the specified stringelement is a part of the specified string list
 */
#define contains( str,... ) contains_internal( str, __VA_ARGS__, (void *)0 )
/**
   Concatenate all the specified strings into a single newly allocated one
 */
#define wcsdupcat( str,... ) wcsdupcat_internal( str, __VA_ARGS__, (void *)0 )

/**
  Print a stack trace to stderr
*/
void show_stackframe();

/**
   Returns a newly allocated wide character string equivalent of the
   specified multibyte character string
*/
wchar_t *str2wcs( const char *in );

/**
   Converts the narrow character string \c in into it's wide
   equivalent, stored in \c out. \c out must have enough space to fit
   the entire string.
*/
wchar_t *str2wcs_internal( const char *in, wchar_t *out );

/**
   Returns a newly allocated multibyte character string equivalent of
   the specified wide character string
*/
char *wcs2str( const wchar_t *in );

/**
   Converts the wide character string \c in into it's narrow
   equivalent, stored in \c out. \c out must have enough space to fit
   the entire string.

   This function decodes illegal character sequences in a reversible
   way using the private use area.
*/
char *wcs2str_internal( const wchar_t *in, char *out );

/**
   Returns a newly allocated wide character string array equivalent of
   the specified multibyte character string array
*/
char **wcsv2strv( const wchar_t **in );

/**
   Returns a newly allocated multibyte character string array equivalent of the specified wide character string array
*/
wchar_t **strv2wcsv( const char **in );

/**
   A call to this function will reset the error counter. Some
   functions print out non-critical error messages. These should check
   the error_count before, and skip printing the message if
   MAX_ERROR_COUNT messages have been printed. The error_reset()
   should be called after each interactive command executes, to allow
   new messages to be printed.
*/
void error_reset();

/**
   This function behaves exactly like a wide character equivalent of
   the C function setlocale.
*/
const wchar_t *wsetlocale( int category, const wchar_t *locale );

/**
   Issue a debug message with printf-style string formating and
   automatic line breaking. The string will begin with the string \c
   program_name, followed by a colon and a whitespace.

   Because debug is often called to tell the user about an error,
   before using wperror to give a specific error message, debug will
   never ever modify the value of errno.
   
   \param level the priority of the message. Lower number means higher priority. Messages with a priority_number higher than \c debug_level will be ignored..
   \param msg the message format string. 

   Example:

   <code>debug( 1, L"Pi = %.3f", M_PI );</code>

   will print the string 'fish: Pi = 3.141', given that debug_level is 1 or higher, and that program_name is 'fish'.
*/
void debug( int level, const wchar_t *msg, ... );

/**
   Print a short message about how to file a bug report to stderr
*/
void bugreport();


#endif

