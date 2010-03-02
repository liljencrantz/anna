#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <time.h>

int duck_debug=0;

int mini(int a, int b)
{
    return (a>b)?b:a;
}

int maxi(int a, int b)
{
    return (a<b)?b:a;
}


struct duck_string
{
  size_t count;
  size_t capacity;
  wchar_t *str;
}
  ;
typedef struct duck_string duck_string_t;

void duck_string_ensure_capacity(duck_string_t *string, size_t size)
{
    if(string->capacity < size)
    {
      string->capacity=maxi(string->capacity*2,size);
      string->str = realloc(string->str, sizeof(wchar_t)*string->capacity);
      
    }
}

void duck_string_init(duck_string_t *string)
{
    memset(string, 0, sizeof(duck_string_t));
}

void duck_string_init_from_ptr(duck_string_t *string, wchar_t *payload, size_t size)
{
  duck_string_init(string);
  duck_string_ensure_capacity(string, size);
  memcpy(string->str, payload, sizeof(wchar_t)*size);  
  string->count = size;
}

void duck_string_destroy(duck_string_t *string)
{
  free(string->str);
  memset(string, 0, sizeof(duck_string_t));
}

void duck_string_append(duck_string_t *dest, duck_string_t *src, size_t offset, size_t length)
{
  duck_string_ensure_capacity(dest, length+dest->count);
  memcpy(&dest->str[dest->count], &src->str[offset], sizeof(wchar_t)*length);  
  dest->count += length;
}

void duck_string_substring(duck_string_t *dest, duck_string_t *src, size_t offset, size_t length)
{
  dest->count=0;
  duck_string_append(dest, src, offset, length);
}

void duck_string_set_char(duck_string_t *dest, size_t offset, wchar_t ch)
{
  dest->str[offset] = ch;
}

wchar_t duck_string_get_char(duck_string_t *dest, size_t offset)
{
  return dest->str[offset];
}

void duck_string_replace(duck_string_t *dest, duck_string_t *src, 
			 size_t dest_offset,  size_t dest_length, 
			 size_t src_offset,   size_t src_length)
{
  duck_string_ensure_capacity(dest, dest->count+src_length-dest_length+10);
  if(dest->count>dest_offset+dest_length)
    memmove(&dest->str[dest_offset+src_length], 
	    &dest->str[dest_offset+dest_length], 
	    sizeof(wchar_t)*(dest->count-dest_offset-dest_length));  
  memcpy(&dest->str[dest_offset], 
	 &src->str[src_offset], 
	 sizeof(wchar_t)*(src_length));
  dest->count += src_length - dest_length;
}

size_t duck_string_get_length(duck_string_t *dest)
{
  return dest->count;
}

void duck_string_truncate(duck_string_t *dest, size_t length)
{
  dest->count = mini(length, dest->count);
}

void duck_string_print(duck_string_t *string)
{
  wprintf(L"%.*ls\n", string->count, string->str);
}
