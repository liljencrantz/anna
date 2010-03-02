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

void duck_string_append(duck_string_t *dest, duck_string_t *src, size_t offset, size_t length);

int mini(int a, int b)
{
    return (a>b)?b:a;
}

int maxi(int a, int b)
{
    return (a<b)?b:a;
}


static void duck_element_disown(duck_string_element_t *el)
{
    el->users--;
    if(el->users == 0) 
    {
	free(el);
    }
}

static void duck_element_use(duck_string_element_t *el)
{
    el->users++;
}


static duck_string_ensure_element_capacity(duck_string_t *string, size_t count)
{
    if(string->element_capacity >= count)
	return;
    
    if(string->element_capacity==0)
    {
	count = maxi(count, DUCK_STRING_DEFAULT_ELEMENT_CAPACITY);
	
	string->element_capacity=count;
	string->element = malloc((sizeof(duck_string_element_t)+sizeof(size_t *)*2)*string->element_capacity);
	string->element_offset = string->element + string->element_capacity;
	string->element_length = string->element_offset + string->element_capacity;  
    }
    else 
    {
	count = maxi(count, DUCK_STRING_DEFAULT_ELEMENT_CAPACITY);
	duck_string_element_t *new_element = malloc((sizeof(duck_string_element_t)+sizeof(size_t *)*2)*count);
	size_t *new_element_offset = new_element + count;
	size_t *new_element_length = new_element_offset + count;
	memcpy(new_element, string->element, sizeof(duck_string_element_t *)*string->element_count);
	memcpy(new_element_offset, string->element_offset, sizeof(size_t)*string->element_count);
	memcpy(new_element_length, string->element_length, sizeof(size_t)*string->element_count);
	free(string->element);
	string->element = new_element;
	string->element_offset = new_element_offset;
	string->element_length = new_element_length;
	string->element_capacity = count;
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
    duck_string_init(string);
    
    duck_string_ensure_element_capacity(string, DUCK_STRING_DEFAULT_ELEMENT_CAPACITY);
    string->element_count=1;
    string->element[0] = duck_string_element_create(payload, size);
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
    size_t dest_base_count = dest->element_count;
    duck_string_ensure_element_capacity(dest, dest->element_count + src->element_count);

    for(i=0;i<src->element_count; i++) {
	size_t last_in_element = first_in_element + src->element_length[i];
	if(last_in_element >offset){
	    int j;
	    size_t additional_offset = offset-first_in_element;
/*	    wprintf(L"Start copying at element %d, position %d+%d, length %d!\n", 
		    i, src->element_offset[i], additional_offset, 
		    mini(src->element_length[i]-additional_offset, length));
*/
	    dest->element[dest_base_count]=src->element[i];
	    dest->element_offset[dest_base_count] = src->element_offset[i] + additional_offset;
	    dest->element_length[dest_base_count] = mini(src->element_length[i]-additional_offset, length);
	    
	    duck_element_use(src->element[i]);
	    dest->element_count++;
	    size_t length_done = dest->element_length[dest_base_count];
	    
	    if(length_done < length)
	    {
		for(j=1; (i+j)<src->element_count; j++)
		{
		    duck_element_use(src->element[i+j]);

		    size_t max_length_done = length_done + src->element_length[i+j];

		    dest->element[dest_base_count+j] = src->element[i+j];
		    dest->element_offset[dest_base_count+j] = src->element_offset[i+j];
		    dest->element_count++;
		    
		    if(length <= max_length_done)
		    {
			dest->element_length[dest_base_count+j] = length-length_done;
			break;
		    }
		    dest->element_length[dest_base_count+j] = src->element_length[i+j];
		}
	    }	    
	    break;
	}
	first_in_element=last_in_element;
	
    }
}

static void duck_string_own_element(duck_string_t *dest, int eid)
{
    if(dest->element[eid]->users == 1)
    {
	return;
    }
    //wprintf(L"Make element my own\n");
    
    duck_string_element_t *old = dest->element[eid];

    dest->element[eid] = duck_string_element_create(&dest->element[eid]->payload[dest->element_offset[eid]], dest->element_length[eid]);
    dest->element_offset[eid] = 0;
    duck_element_disown(old);
    
    //wprintf(L"I have my very own %.*ls\n", dest->element_length[eid], dest->element[eid]->payload);
}

void duck_string_set_char(duck_string_t *dest, size_t offset, wchar_t ch)
{
    int i;
    size_t first_in_element=0;
    //wprintf(L"Woo\n");
    
    for(i=0;i<dest->element_count; i++) {
	size_t last_in_element = first_in_element + dest->element_length[i];
	if(last_in_element >offset){
	    duck_string_own_element(dest, i);
	    
	    size_t poff = offset + dest->element_offset[i]-first_in_element;
	    //wprintf(L"poff is %d\n", poff);
	    
	    dest->element[i]->payload[poff] = ch;
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
	if(last_in_element >length){
	    dest->element_length[i] = length - first_in_element;
	    int j;
	    for(j=i+1; j<dest->element_count; j++)
	    {
		duck_element_disown(dest->element[j]);
	    }
	    dest->element_count = i+1;
	}
	first_in_element = last_in_element;
    } 
    
}

void duck_string_print(duck_string_t *dest)
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

