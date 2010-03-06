#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <time.h>

int anna_debug=0;

int mini(int a, int b)
{
    return (a>b)?b:a;
}

int maxi(int a, int b)
{
    return (a<b)?b:a;
}


struct anna_string
{
  size_t count;
  size_t capacity;
  wchar_t *str;
}
  ;
typedef struct anna_string anna_string_t;

void anna_string_ensure_capacity(anna_string_t *string, size_t size)
{
    if(string->capacity < size)
    {
      string->capacity=maxi(string->capacity*2,size);
      string->str = realloc(string->str, sizeof(wchar_t)*string->capacity);      
    }
}

void anna_string_init(anna_string_t *string)
{
    memset(string, 0, sizeof(anna_string_t));
}

void anna_string_init_from_ptr(anna_string_t *string, wchar_t *payload, size_t size)
{
  anna_string_init(string);
  anna_string_ensure_capacity(string, size);
  memcpy(string->str, payload, sizeof(wchar_t)*size);  
  string->count = size;
}

void anna_string_destroy(anna_string_t *string)
{
  free(string->str);
  memset(string, 0, sizeof(anna_string_t));
}

void anna_string_append(anna_string_t *dest, anna_string_t *src, size_t offset, size_t length)
{
  anna_string_ensure_capacity(dest, length+dest->count);
  memcpy(&dest->str[dest->count], &src->str[offset], sizeof(wchar_t)*length);  
  dest->count += length;
}

void anna_string_substring(anna_string_t *dest, anna_string_t *src, size_t offset, size_t length)
{
  dest->count=0;
  anna_string_append(dest, src, offset, length);
}

void anna_string_set_char(anna_string_t *dest, size_t offset, wchar_t ch)
{
  dest->str[offset] = ch;
}

wchar_t anna_string_get_char(anna_string_t *dest, size_t offset)
{
  return dest->str[offset];
}

void anna_string_replace(anna_string_t *dest, anna_string_t *src, 
			 size_t dest_offset,  size_t dest_length, 
			 size_t src_offset,   size_t src_length)
{
  anna_string_ensure_capacity(dest, dest->count+src_length-dest_length+10);
  if(dest->count>dest_offset+dest_length)
    memmove(&dest->str[dest_offset+src_length], 
	    &dest->str[dest_offset+dest_length], 
	    sizeof(wchar_t)*(dest->count-dest_offset-dest_length));  
  memcpy(&dest->str[dest_offset], 
	 &src->str[src_offset], 
	 sizeof(wchar_t)*(src_length));
  dest->count += src_length - dest_length;
}

size_t anna_string_get_length(anna_string_t *dest)
{
  return dest->count;
}

void anna_string_truncate(anna_string_t *dest, size_t length)
{
  dest->count = mini(length, dest->count);
}

void anna_string_print(anna_string_t *string)
{
  wprintf(L"%.*ls\n", string->count, string->str);
}
