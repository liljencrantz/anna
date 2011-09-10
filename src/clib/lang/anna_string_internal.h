#ifndef ANNA_STRING_INTERNAL_H
#define ANNA_STRING_INTERNAL_H

#include "anna_checks.h"

#define ANNA_STRING_INTERNAL 8

struct anna_string
{
    size_t count;
    size_t capacity;
    wchar_t *str;
    wchar_t internal[ANNA_STRING_INTERNAL];
}
    ;
typedef struct anna_string anna_string_t;

/**
   Init the string to an empty value
 */
void asi_init(anna_string_t *string);
/**
   Init the string to contain the specified C string
 */
void asi_init_from_ptr(
    anna_string_t *string, wchar_t *payload, size_t size);
/**
  Destroy the specified string, free:ing all memory used by it.
*/
void asi_destroy(anna_string_t *string);

/**
   Append the specified substring to the end of the specified string
 */
void asi_append(
    anna_string_t *dest, anna_string_t *src,
    size_t offset, size_t count);

/**
   Copies the specified substring into the specified string
 */
void asi_substring(
    anna_string_t *dest, anna_string_t *src,
    size_t offset, size_t count);

/**
   Sets the character at the specified offset of the string
 */
void asi_set_char(
    anna_string_t *dest, size_t offset, wchar_t ch);

/**
   Returns the character at the specified offset of the string
 */
wchar_t asi_get_char(
    anna_string_t *dest, size_t offset);

/**
   Replace the pecified substring in source with the specified substring of dest
 */
void asi_replace(
    anna_string_t *dest, anna_string_t *src, 
    size_t dest_offset, size_t dest_count, 
    size_t src_offset, size_t src_count);
/**
   Return the character count of the string
 */
size_t asi_get_count(anna_string_t *str);

void asi_truncate(
    anna_string_t *str, size_t count);

/**
   Print the specified string, together with a load of debug and
   status information about it, on stdout.

   For debugging purposes only.
 */
void asi_print_debug(anna_string_t *str);

/**
   Print the specified string.
 */
void asi_print_regular(anna_string_t *str);

wchar_t *asi_cstring(anna_string_t *str);

char *asi_cstring_narrow(
    anna_string_t *str);

/**
   Compare the two specified strings
 */
int asi_compare(
    anna_string_t *a, anna_string_t *b);

void asi_append_cstring(
    anna_string_t *a, wchar_t *str, size_t len);

#endif
