#ifndef ANNA_STRING_INTERNAL_H
#define ANNA_STRING_INTERNAL_H

#include "anna_checks.h"

#ifdef ANNA_STRING_CHUNKED_ENABLED

struct anna_string_element;

typedef struct 
{
    size_t element;
    size_t offset;
}
    anna_string_location_t;

struct anna_string
{
    /**
      The number of string_element objects used in theis string
     */
    size_t element_count;
    /**
       The number of string_element slots allocated for usin in this string
     */
    size_t element_capacity;
    /**
       Array of string_element objects
     */
    struct anna_string_element **element;
    /**
       The offset of the specified string element at which this string
       starts using the specified string element.
     */
    size_t *element_offset;
    /**
       The number of characters of data used in this string element
     */
    size_t *element_length;
    /**
       Total string length
     */
    size_t length;
    /**
       The index of the position that was last accessed
     */
    size_t cache_pos;
    /**
       Element index and offset of the position that was last accessed
     */
    anna_string_location_t cache_value;
}
  ;

typedef struct anna_string anna_string_t;

#else

struct anna_string
{
  size_t count;
  size_t capacity;
  wchar_t *str;
}
  ;
typedef struct anna_string anna_string_t;

#endif


/**
   Init the string to an empty value
 */
void anna_string_init(anna_string_t *string);
/**
   Init the string to contain the specified C string
 */
void anna_string_init_from_ptr(anna_string_t *string, wchar_t *payload, size_t size);
/*
  Destroy the specified string, free:ing all memory used by it.
 */
void anna_string_destroy(anna_string_t *string);

/**
   Append the specified substring to the end of the specified string
 */
void anna_string_append(anna_string_t *dest, anna_string_t *src, size_t offset, size_t length);

/**
   Copies the specified substring into the specified string
 */
void anna_string_substring(anna_string_t *dest, anna_string_t *src, size_t offset, size_t length);

/**
   Sets the character at the specified offset of the string
 */
void anna_string_set_char(anna_string_t *dest, size_t offset, wchar_t ch);
/**
   Returns the character at the specified offset of the string
 */
wchar_t anna_string_get_char(anna_string_t *dest, size_t offset);

/**
   Replace the pecified substring in source with the specified substring of dest
 */
void anna_string_replace(anna_string_t *dest, anna_string_t *src, size_t dest_offset, size_t dest_length, size_t src_offset, size_t src_length);
/**
   Return the length of the string
 */
size_t anna_string_get_length(anna_string_t *dest);

void anna_string_truncate(anna_string_t *dest, size_t length);

/**
   Print the specified string, together with a load of debug and
   status information about it, on stdout.

   For debugging purposes only.
 */
void anna_string_print_regular(anna_string_t *dest);

#endif
