/** \file common.h
	Prototypes for various functions, mostly string utilities, that are used by most parts of anna.
*/

#ifndef ANNA_COMMON_H
/**
   Header guard
*/
#define ANNA_COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#include "anna/util.h"

/**
   Maximum number of bytes used by a single utf-8 character
*/
#define MAX_UTF8_BYTES 6

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
   Information that is likely to be of little use except in debugging
 */
#define D_SPAM 0
/**
   Info is potentially relevant information about the status of the app
 */
#define D_INFO 1
/**
   A warning is issued after a recoverable error is encountered.
 */
#define D_WARNING 2
/**
   Errors are conditions that will lead to application failiure. The
   program will keep running in order to try and shut down gracefully
   and to give as much additional information as possible.
 */
#define D_ERROR 3
/**
   Critical errors are errors that lead to the application exiting at once. 
 */
#define D_CRITICAL 4


/**
   The verbosity level. If a call to debug has a severity
   level higher than or equal to \c debug_level, it will be printed.
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
#define VERIFY( arg, retval )						\
    if( !(arg) )							\
    {									\
	debug( 0,							\
	        L"function %s called with null value for argument %s. " , \
	       __func__,						\
	       #arg );							\
	show_stackframe();						\
	return retval;							\
    }

/**
   Pause for input, then exit the program. If supported, print a backtrace first.
*/
#define FATAL_EXIT() exit(1)	

/**
   Exit program at once, leaving an error message about running out of memory.
*/
#define DIE_MEM()							\
    {									\
	fwprintf( stderr,						\
		  L"anna: Out of memory on line %d of file %s, shutting down.\n", \
		  __LINE__,						\
		  __FILE__ );						\
	FATAL_EXIT();							\
    }

/**
  Print a stack trace to stderr
*/
void show_stackframe(void);

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
*/
void debug( int level, const wchar_t *msg, ... );
void anna_message(const wchar_t *msg, ... );
void anna_message_set_buffer(string_buffer_t *message_buffer);

/**
   Low level printing function. Convert the specified string to a
   narrow string and write it to the specified file descriptor. If len
   is -1, the string is assumed to be null-terminated, otherwise, len
   is used to determine its length.
 */
void anna_print(int fd, wchar_t *str, ssize_t len);

#endif

