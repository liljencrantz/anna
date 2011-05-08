#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#include "util.h"
#include "common.h"
#include "anna_checks.h"
#include "anna_crash.h"
#include "anna_string_internal.h"

#ifndef ANNA_STRING_CHUNKED_ENABLED

void asi_ensure_capacity(anna_string_t *string, size_t size)
{
    if(string->capacity < size)
    {
      string->capacity=maxi(string->capacity*2,size);
      string->str = realloc(string->str, sizeof(wchar_t)*string->capacity);      
    }
}

void asi_init(anna_string_t *string)
{
    memset(string, 0, sizeof(anna_string_t));
}

void asi_init_from_ptr(anna_string_t *string, wchar_t *payload, size_t size)
{
  asi_init(string);
  asi_ensure_capacity(string, size);
  memcpy(string->str, payload, sizeof(wchar_t)*size);  
  string->count = size;
}

void asi_destroy(anna_string_t *string)
{
  free(string->str);
  memset(string, 0, sizeof(anna_string_t));
}

void asi_append(anna_string_t *dest, anna_string_t *src, size_t offset, size_t length)
{
  asi_ensure_capacity(dest, length+dest->count);
  memcpy(&dest->str[dest->count], &src->str[offset], sizeof(wchar_t)*length);  
  dest->count += length;
}

void asi_substring(anna_string_t *dest, anna_string_t *src, size_t offset, size_t length)
{
  dest->count=0;
  asi_append(dest, src, offset, length);
}

void asi_set_char(anna_string_t *dest, size_t offset, wchar_t ch)
{
  dest->str[offset] = ch;
}

wchar_t asi_get_char(anna_string_t *dest, size_t offset)
{
  return dest->str[offset];
}

void asi_replace(
    anna_string_t *dest, anna_string_t *src, 
    size_t dest_offset,  size_t dest_length, 
    size_t src_offset,   size_t src_length)
{
  asi_ensure_capacity(dest, dest->count+src_length-dest_length+10);
  if(dest->count>dest_offset+dest_length)
    memmove(&dest->str[dest_offset+src_length], 
	    &dest->str[dest_offset+dest_length], 
	    sizeof(wchar_t)*(dest->count-dest_offset-dest_length));  
  memcpy(&dest->str[dest_offset], 
	 &src->str[src_offset], 
	 sizeof(wchar_t)*(src_length));
  dest->count += src_length - dest_length;
}

size_t asi_get_length(anna_string_t *dest)
{
  return dest->count;
}

void asi_truncate(anna_string_t *dest, size_t length)
{
  dest->count = mini(length, dest->count);
}

void asi_print(anna_string_t *string)
{
  wprintf(L"%.*ls\n", string->count, string->str);
}

void asi_print_regular(anna_string_t *string)
{
    wprintf(L"%.*ls\n", string->count, string->str);
}

void asi_print_debug(anna_string_t *str)
{
    CRASH;
}

wchar_t *asi_cstring(anna_string_t *str)
{
    CRASH;
}

/**
   Compare the two specified strings
 */
int asi_compare(anna_string_t *a, anna_string_t *b)
{
    size_t sz = mini(a->count, b->count);
    int res = wcsncmp(a->str, b->str, sz);
    if(res == 0)
    {
	return a->count-b->count;
    }
    return res;
}

void asi_append_cstring(anna_string_t *dest, wchar_t *str, size_t len)
{
    asi_ensure_capacity(dest, dest->count + len);
    memcpy(&dest->str[dest->count], str, sizeof(wchar_t)*len);  
}



#endif
