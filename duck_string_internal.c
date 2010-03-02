#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#include "duck_string_internal.h"

/**
   Wchar_t-using, chunk based string implementation, does most string operations in O(1). Most of the time. :)
*/

#define DUCK_STRING_DEFAULT_ELEMENT_CAPACITY 12
#define DUCK_STRING_APPEND_SHORT_LIMIT 16

#define DUCK_STRING_HAIRCUT_RATIO 8

#define CRASH					\
    {						\
    int *__tmp=0;				\
    *__tmp=0;					\
    }

int duck_debug;


struct duck_string_element
{
    int users;
    size_t capacity;
    wchar_t payload[];
}
  ;

typedef struct duck_string_element duck_string_element_t;

duck_string_element_t *duck_string_element_create(wchar_t *payload, size_t size, size_t available);
static void duck_string_print(duck_string_t *dest);


int mini(int a, int b)
{
    return (a>b)?b:a;
}

int maxi(int a, int b)
{
    return (a<b)?b:a;
}

/**
   Decrease the number of owners on the specified element. If the
   number of owners is zero, kill it dead.
 */
static void duck_element_disown(duck_string_element_t *el)
{
    el->users--;
    if(el->users == 0) 
    {
	free(el);
    }
}

/**
   Increase the number of owners on the specified element.
 */
static void duck_element_adopt(duck_string_element_t *el)
{
    el->users++;
}

/**
  Make sure the specified element of the specified string only has a
  single owner, and that it has at least the specified amount of
  unused capacity. If not, replace it with a copy.
*/
static void duck_string_element_stepford(duck_string_t *dest, int eid, size_t min_available)
{
  
    if(dest->element[eid]->users == 1)
    {
        size_t available = dest->element[eid]->capacity - (dest->element_length[eid]+dest->element_offset[eid]);
	if(available >= min_available)
	    return;
	size_t increment = 3*(min_available - available);
      
	dest->element[eid] = realloc(dest->element[eid], sizeof(duck_string_element_t) + sizeof(wchar_t)*(dest->element[eid]->capacity + increment));
	dest->element[eid]->capacity += increment;
    }
    //wprintf(L"Make element my own\n");
    
    duck_string_element_t *old = dest->element[eid];

    dest->element[eid] = duck_string_element_create(&dest->element[eid]->payload[dest->element_offset[eid]], dest->element_length[eid], min_available*3);
    dest->element_offset[eid] = 0;
    duck_element_disown(old);
    
    //wprintf(L"I have my very own %.*ls\n", dest->element_length[eid], dest->element[eid]->payload);
}

/**
   Trim away short string elements in order to tidy up the
   string. Sort of a string defragmentation. 
*/
static void duck_string_haircut(duck_string_t *hippie)
{
    
  int i, j, k, m;
  for(i=0; i<hippie->element_count; i++)
  {
      size_t len = hippie->element_length[i];
      size_t count = 1;
      if(len / (count * DUCK_STRING_HAIRCUT_RATIO) != 0) 
	{
	  continue;
	}
      for(j=i+1; j<hippie->element_count; j++)
	{
	  if((len+ hippie->element_length[j]) / ((count+1) * DUCK_STRING_HAIRCUT_RATIO) != 0) 
	    break;
	  len += hippie->element_length[j];
	  count++;	
	}
      if(count > 3) 
	{
	  size_t old_length = duck_string_get_length(hippie);
	  
	  size_t pos = hippie->element_length[i];
	  duck_string_element_t *el = duck_string_element_create(&hippie->element[i]->payload[hippie->element_offset[i]], hippie->element_length[i], len - hippie->element_length[i]);
	  for(k=i+1;k<j;k++)
	    {
	      memcpy(&el->payload[pos],&hippie->element[k]->payload[hippie->element_offset[k]],sizeof(wchar_t)*hippie->element_length[k]);
	      duck_element_disown(hippie->element[k]);
	      pos +=hippie->element_length[k];	      
	    }
	  //assert(pos == len);
	  
	  hippie->element[i] = el;
	  hippie->element_length[i] = len;
	  hippie->element_offset[i] = 0;
	  memmove(&hippie->element[i+1], &hippie->element[j], sizeof(duck_string_element_t *)*(hippie->element_count-j));
	  memmove(&hippie->element_length[i+1], &hippie->element_length[j], sizeof(size_t)*(hippie->element_count-j));
	  memmove(&hippie->element_offset[i+1], &hippie->element_offset[j], sizeof(size_t)*(hippie->element_count-j));
	  hippie->element_count -= (j-i-1);

	  //assert(old_length == duck_string_get_length(hippie));
	  /*
	  for(m=0; m<hippie->element_count; m++)
	    {
	      assert(hippie->element[m]);
	      assert(hippie->element[m]->payload);
	      assert(hippie->element[m]->capacity >= (hippie->element_length[m]+hippie->element_offset[m]));
	    }

	  for(m=hippie->element_count; m<hippie->element_capacity; m++)
	    {
	      hippie->element_length[m] = 0;
	      hippie->element_offset[m] = 0;
	      hippie->element[m] = 0;
	    }
	  */
	  return;
	  
	}
  }
}


static duck_string_ensure_element_capacity(duck_string_t *string, size_t count)
{
    if(string->element_capacity >= count)
      {
	return;
      }

    if(string->element_capacity==0)
    {
      count = maxi(count*2, DUCK_STRING_DEFAULT_ELEMENT_CAPACITY);
      
	string->element_capacity=count;
	string->element = malloc((sizeof(duck_string_element_t*)+sizeof(size_t)*2)*count);
	string->element_offset = string->element + count;
	string->element_length = string->element_offset + count;
    }
    else 
    {
      
        count = maxi(string->element_capacity*2, count+ DUCK_STRING_DEFAULT_ELEMENT_CAPACITY);
      if(duck_debug)
	{
	  //	  wprintf(L"LALA changing size from %d to %d\n", string->element_capacity, count);
	}

	if(1)
	  {
	    
	duck_string_element_t *new_element = malloc((sizeof(duck_string_element_t*)+sizeof(size_t)*2)*count);
	size_t *new_element_offset = ((size_t *)new_element) + count;
	size_t *new_element_length = new_element_offset + count;
	memcpy(new_element, string->element, sizeof(duck_string_element_t *)*string->element_count);
	memcpy(new_element_offset, string->element_offset, sizeof(size_t)*string->element_count);
	memcpy(new_element_length, string->element_length, sizeof(size_t)*string->element_count);
	free(string->element);
	string->element = new_element;
	string->element_offset = new_element_offset;
	string->element_length = new_element_length;
	  }
	else 
	  {
	    

	duck_string_element_t *new_element = realloc(string->element,(sizeof(duck_string_element_t*)+sizeof(size_t)*2)*count);
	size_t *new_element_offset = ((size_t *)new_element) + count;
	size_t *new_element_length = new_element_offset + count;
	//memcpy(new_element, string->element, sizeof(duck_string_element_t *)*string->element_count);
	memmove(new_element_length, (void *)string->element_length-(void *)string->element+(void *)new_element, sizeof(size_t)*string->element_count);
	memmove(new_element_offset, (void *)string->element_offset-(void *)string->element+(void *)new_element, sizeof(size_t)*string->element_count);
	string->element = new_element;
	string->element_offset = new_element_offset;
	string->element_length = new_element_length;
	  }
	
	string->element_capacity = count;
    }
}

duck_string_element_t *duck_string_element_create(wchar_t *payload, size_t size, size_t available)
{
    duck_string_element_t *res = malloc(sizeof(duck_string_element_t) + sizeof(wchar_t)*(size+available));
    res->users=1;
    res->capacity=size+available;
    memcpy(&res->payload[0], payload, sizeof(wchar_t)*size);
    return res;
}

void duck_string_init(duck_string_t *string)
{
  memset(string, 0, sizeof(duck_string_t));
}

void duck_string_init_from_ptr(duck_string_t *string, wchar_t *payload, size_t size)
{
    duck_string_init(string);
    
    duck_string_ensure_element_capacity(string, DUCK_STRING_DEFAULT_ELEMENT_CAPACITY);
    string->element_count=1;
    string->element[0] = duck_string_element_create(payload, size, 5);
    string->element_offset[0]=0;
    string->element_length[0]=size;
}

void duck_string_destroy(duck_string_t *string)
{
    int i;
    for(i=0;i<string->element_count; i++) {
	duck_element_disown(string->element[i]);
    }
    free(string->element);
}


void duck_string_substring(duck_string_t *dest, duck_string_t *src, size_t offset, size_t length)
{
    duck_string_truncate(dest, 0);
    duck_string_append(dest, src, offset, length);
}

void duck_string_append(duck_string_t *dest, duck_string_t *src, size_t offset, size_t length)
{
    int i;
    size_t first_in_element=0;
    //    wprintf(L"Append element to string with %d elements\n", dest->element_count);
    
    if(src->element_count == 1)
      {
	//	wprintf(L"FAST\n");
	
	duck_string_ensure_element_capacity(dest, dest->element_count + 1);
	dest->element[dest->element_count] = src->element[0];
	dest->element_length[dest->element_count] = length;
	dest->element_offset[dest->element_count] = src->element_offset[0]+offset;
	dest->element[dest->element_count]->users++;
	dest->element_count++;	
	return;    
      }
    
    //wprintf(L"SLOW\n");
    
    if(length < DUCK_STRING_APPEND_SHORT_LIMIT) 
    {
	if(dest->element_count == 0)
	{
	    duck_string_ensure_element_capacity(dest, 1);
	    dest->element_count=1;	    
	    dest->element[0] = duck_string_element_create(0,0, length);	    
	    dest->element_length[0]=0;
	    dest->element_offset[0]=0;	    
	}
	else
	{
	    duck_string_element_stepford(dest, dest->element_count-1, length);
	}
	
	duck_string_element_t *el = dest->element[dest->element_count-1];
	size_t dest_offset = dest->element_offset[dest->element_count-1]+dest->element_length[dest->element_count-1];
	for(i=0; i<length;i++)
	{
	    el->payload[dest_offset+i] = duck_string_get_char(src, i+offset);	    
	}
	dest->element_length[dest->element_count-1] += length;
	return;
      }
    


    duck_string_haircut(src);
    duck_string_haircut(dest);
    
    if(length == 0) 
    {
      return;
    }
    duck_string_ensure_element_capacity(dest, dest->element_count + src->element_count);
    size_t dest_base_count = dest->element_count;
    
    for(i=0;i<src->element_count; i++) {
	size_t last_in_element = first_in_element + src->element_length[i];
	if(last_in_element >offset)
        {
	    int j;
	    size_t additional_offset = offset-first_in_element;
/*	    wprintf(L"Start copying at element %d, position %d+%d, length %d!\n", 
		    i, src->element_offset[i], additional_offset, 
		    mini(src->element_length[i]-additional_offset, length));
*/
	    dest->element[dest_base_count]=src->element[i];
	    dest->element_offset[dest_base_count] = src->element_offset[i] + additional_offset;
	    assert(dest_base_count < dest->element_capacity);
	    
	    dest->element_length[dest_base_count] = mini(src->element_length[i]-additional_offset, length);
	    
	    duck_element_adopt(src->element[i]);
	    dest->element_count++;
	    size_t length_done = dest->element_length[dest_base_count];
	    assert(length_done);
	    
	    if(length_done < length)
	    {
		for(j=1; (i+j)<src->element_count; j++)
		{
		    duck_element_adopt(src->element[i+j]);

		    size_t max_length_done = length_done + src->element_length[i+j];

		    dest->element[dest_base_count+j] = src->element[i+j];
		    dest->element_offset[dest_base_count+j] = src->element_offset[i+j];
		    dest->element_count++;
		    
		    if(length <= max_length_done)
		    {
			dest->element_length[dest_base_count+j] = length-length_done;
			return;
		    }
		    dest->element_length[dest_base_count+j] = src->element_length[i+j];
		    length_done += src->element_length[i+j];
		}
	    }	    
	    break;
	}
	first_in_element=last_in_element;
	
    }
}

void duck_string_set_char(duck_string_t *dest, size_t offset, wchar_t ch)
{
    int i;
    size_t first_in_element=0;
    //wprintf(L"Woo\n");
    
    for(i=0;i<dest->element_count; i++) {
	size_t last_in_element = first_in_element + dest->element_length[i];
	if(last_in_element >offset){
	  duck_string_element_stepford(dest, i, 0);
	    
	    size_t poff = offset + dest->element_offset[i]-first_in_element;
	    //wprintf(L"poff is %d\n", poff);
	    
	    dest->element[i]->payload[poff] = ch;
	    return;
	}
	first_in_element = last_in_element;
    }
    wprintf(L"Error: Tried to set element %d in string of length %d\n", offset, first_in_element);  
    CRASH;
    
}

wchar_t duck_string_get_char(duck_string_t *dest, size_t offset)
{
    int i;
    size_t first_in_element=0;
    
    for(i=0;i<dest->element_count; i++) {
	size_t last_in_element = first_in_element + dest->element_length[i];
	if(last_in_element >offset){
	    return dest->element[i]->payload[offset + dest->element_offset[i]-first_in_element];
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

void duck_string_truncate(duck_string_t *dest, size_t length)
{
    int i;
    size_t first_in_element=0;
    
    for(i=0;i<dest->element_count; i++) {
	size_t last_in_element = first_in_element + dest->element_length[i];
	if(last_in_element >=length){
	    dest->element_length[i] = length - first_in_element;
	    int j;
	    for(j=i+1; j<dest->element_count; j++)
	    {
		duck_element_disown(dest->element[j]);
	    }
	    dest->element_count = i+1;
	    return;
	    
	}
	first_in_element = last_in_element;
    } 
    
}

void duck_string_replace(duck_string_t *dest, 
			 duck_string_t *src,
			 size_t dest_offset, 
			 size_t dest_length,
			 size_t src_offset, 
			 size_t src_length)
{
  /*
    Create a temporary empty string to add things to
   */
  duck_string_t tmp;
  duck_string_init(&tmp);
  
  /*
    Add all the specified bits to the temporary string
   */
  duck_string_ensure_element_capacity(&tmp, dest->element_capacity + src->element_count);
  duck_string_append(&tmp, dest, 0, dest_offset);
  duck_string_append(&tmp, src, src_offset, src_length);
  duck_string_append(&tmp, dest, dest_offset+dest_length, duck_string_get_length(dest)-dest_offset-dest_length);
  
  /*
    Replace the destination string with the temporary one
   */
  duck_string_destroy(dest);
  memcpy(dest, &tmp, sizeof(duck_string_t));
}

/**
   Print the specified string, together with a load of debug and
   status information about it, on stdout.

   For debugging purposes only.
 */
static void duck_string_print(duck_string_t *dest)
{
    //wprintf(L"Print string with %d elements\n", dest->element_count);
    
    int i;
    for(i=0;i<dest->element_count; i++) {
	wprintf(L"%.*ls", dest->element_length[i], &(dest->element[i]->payload[dest->element_offset[i]]));
    }
    wprintf(L"\n");
    
    for(i=0;i<dest->element_count; i++) {
	int j;
	for(j=0;j<dest->element_length[i]; j++)
	{
	    wprintf(L"%d", i+1);
	}
    }
    wprintf(L"\n");
    for(i=0;i<duck_string_get_length(dest); i++) {
	wprintf(L"%d", i%10);
    }
    wprintf(L"\n");
}

