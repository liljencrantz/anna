#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

/**
   Wchar_t-using, chunk based string implementation, does most string operations in O(1). Most of the time. :)
 */

#define DUCK_STRING_DEFAULT_ELEMENT_CAPACITY 3

#define CRASH					\
    {						\
    int *__tmp=0;				\
    *__tmp=0;					\
    }

struct duck_string_element
{
  int users;
  size_t capacity;
  wchar_t payload[];
}
  ;

typedef struct duck_string_element duck_string_element_t;
typedef struct duck_string duck_string_t;

void duck_string_append(duck_string_t *dest, duck_string_t *src, size_t offset, size_t length);


static duck_string_ensure_element_capacity(duck_string_t *string, size_t count)
{
  if(string->element_capacity >= count)
    return;
  
  if(string->element_capacity==0)
    {
      string->element_capacity=count;
      string->element = malloc((sizeof(duck_string_element_t)+sizeof(size_t *)*2)*string->element_capacity);
      string->element_offset = string->element + sizeof(size_t *)*string->element_capacity;
      string->element_length = string->element_offset + sizeof(size_t *)*string->element_capacity;  
    }
  else 
    {
      wprintf(L"Increasing capacity not yet supported. :-(\n");
      CRASH;
    }
}

duck_string_element_t *duck_string_element_create(wchar_t *payload, size_t size)
{
  duck_string_element_t *res = malloc(sizeof(duck_string_element_t) + sizeof(wchar_t)*size);
  res->users=1;
  res->capacity=size;
  memcpy(&res->payload[0], payload, sizeof(wchar_t)*size);
  return res;
}

void duck_string_init(duck_string_t *string)
{
  memset(string, 0, sizeof(duck_string_t));
}

void duck_string_init_from_string(duck_string_t *string, wchar_t *payload, size_t size)
{
  assert(string->element_capacity==0);
  duck_string_ensure_element_capacity(string, DUCK_STRING_DEFAULT_ELEMENT_CAPACITY);
  string->element_count=1;
  string->element[0] = duck_string_element_create(payload, size);
  string->element_offset[0]=0;
  string->element_length[0]=size;
}

void duc_string_clear(duck_string_t *dest)
{
  int i;
  for(i=0;i<dest->element_count; i++) {
    duck_string_discard(dest->element[i]);
  }
  dest->element_count=0;
}

void duck_string_substring(duck_string_t *dest, duck_string_t *src, size_t offset, size_t length)
{
  duck_string_clear(dest);
  duck_string_append(dest, src, offset, length);
  
}

void duck_string_append(duck_string_t *dest, duck_string_t *src, size_t offset, size_t length)
{
  int i;
  size_t first_in_element=0;
  size_t dest_base_count = dest->element_count;
  
  duck_string_ensure_element_capacity(dest, dest->element_capacity + src->element_capacity);
  for(i=0;i<src->element_count; i++) {
    size_t last_in_element = first_in_element + dest->element_length[i];
    if(last_in_element >offset){
      int j;
      dest->element[dest_base_count]=src->element[i];
      dest->element_offset[dest_base_count] = offset-first_in_element;
      dest->element_length[dest_base_count] = mini(src->element_length[i] - dest->element_offset[0], length);
      duck_string_use(src->element[i]);
      
      size_t length_done = dest->element_length[dest_base_count];
      for(j=1; j<src_element_count-i; j++)
	{
	  duck_string_use(src->element[i+j]);
	  size_t max_length_done = length_done + dest->element_length[i+j];
	  dest->element[dest_base_count+j]=src->element[i+j];
	  dest->element_offset[dest_base_count+j] = src->element_offset[i+j];
	  if(length <= max_length_done)
	    {
	      dest->element_length[dest_base_count+j] = length-length_done;
	      break;
	    }
	  dest->element_length[dest_base_count+j] = src->element_length[i+j];
	}
      break;
    }
  }
}

static void duck_string_own_element(duck_string_t *dest, int eid)
{
  if(dest->element[eid]->users == 1)
    {
      return;
    }
  dest->element[eid] = duck_string_element_create(&dest->element[eid]->payload[dest->element_offset], dest->element_length);
  dest->element_offset = 0;
}

void duck_string_set_char(duck_string_t *dest, size_t offset, wchar_t ch)
{
  int i;
  size_t first_in_element=0;
  
  for(i=0;i<dest->element_count; i++) {
    size_t last_in_element = first_in_element + dest->element_length[i];
    if(last_in_element >offset){
      duck_string_own_element(dest, i);
      dest->element[i]->payload[offset + dest->element_offset[i]] = ch;
      return;
    }
    first_in_element = last_in_element;
  }
  wprintf(L"Error: Tried to set element %d in string of length %d\n", offset, first_in_element);  
}

wchar_t duck_string_get_char(duck_string_t *dest, size_t offset)
{
  int i;
  size_t first_in_element=0;
  
  for(i=0;i<dest->element_count; i++) {
    size_t last_in_element = first_in_element + dest->element_length[i];
    if(last_in_element >offset){
      return dest->element[i]->payload[offset + dest->element_offset[i]];
    }
    first_in_element = last_in_element;
  } 
  wprintf(L"Error: Tried to set element %d in string of length %d\n", offset, first_in_element);  
}

size_t duck_string_get_length(duck_string_t *dest)
{
  int i;
  size_t result=0;
  for(i=0;i<dest->element_count; i++) {
    result += dest->element_length[i];
  }
  return result;
}

void duck_string_print(duck_string_t *dest)
{
  int i;
  for(i=0;i<dest->element_count; i++) {
    wprintf(L"%ls", dest->element->payload);
  }
}

int main()
{
  duck_string_t a, b, c, d, e, f;
  duck_string_init(&a);
  duck_string_init(&e);
  duck_string_init(&f);
  duck_string_init_from_string(&b, L"TRALALA", 7);
  duck_string_init_from_string(&c, L"TRALALA", 0);
  duck_string_init_from_string(&d, L"A\0B\0C\0gggggg", 8);
  duck_string_append(&a, &b, 0, 1);
  duck_string_append(&a, &b, 3, 2);
  duck_string_append(&a, &b, 1, 2);
  duck_string_append(&a, &b, 1, 2);
  duck_string_print(&a);
}

