#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna.h"
#include "anna_node.h"
#include "anna_node_create.h"
#include "anna_buffer.h"
#include "anna_int.h"
#include "anna_stack.h"
#include "anna_type.h"
#include "anna_member.h"
#include "anna_function_type.h"
#include "anna_function.h"
#include "anna_range.h"
#include "anna_vm.h"
#include "anna_list.h"
#include "anna_string.h"
#include "anna_mid.h"
#include "anna_util.h"
#include "anna_intern.h"

#include "anna_macro.h"

anna_object_t *anna_buffer_create()
{
    anna_object_t *obj= anna_object_create(buffer_type);
    (*anna_entry_get_addr(obj,ANNA_MID_BUFFER_PAYLOAD))=0;
    (*(size_t *)anna_entry_get_addr(obj,ANNA_MID_BUFFER_CAPACITY)) = 0;    
    (*(size_t *)anna_entry_get_addr(obj,ANNA_MID_BUFFER_SIZE)) = 0;
    return obj;
}

void anna_buffer_set(struct anna_object *this, ssize_t offset, unsigned char value)
{
    size_t size = anna_buffer_get_count(this);
    ssize_t pos = anna_list_calc_offset(offset, size);
//    wprintf(L"Set el %d in buffer of %d elements\n", pos, size);
    if(pos < 0)
    {
	return;
    }
    if(pos >= size)
    {
	anna_buffer_set_count(this, pos+1);      
    }
    
    unsigned char *ptr = anna_buffer_get_payload(this);
    ptr[pos] = value;  
}

unsigned char anna_buffer_get(anna_object_t *this, ssize_t offset)
{
    size_t size = anna_buffer_get_count(this);
    ssize_t pos = anna_list_calc_offset(offset, size);
    unsigned char *ptr = anna_buffer_get_payload(this);
    if(pos < 0||pos >=size)
    {
	return 0;
    }
    return ptr[pos];
}

size_t anna_buffer_get_count(anna_object_t *this)
{
    return *(size_t *)anna_entry_get_addr(this,ANNA_MID_BUFFER_SIZE);
}

void anna_buffer_set_count(anna_object_t *this, size_t sz)
{
    size_t capacity = anna_buffer_get_capacity(this);
    
    if(sz>capacity)
    {
	anna_buffer_set_capacity(this, sz);
    }
    *(size_t *)anna_entry_get_addr(this,ANNA_MID_BUFFER_SIZE) = sz;
}

size_t anna_buffer_get_capacity(anna_object_t *this)
{
    return *(size_t *)anna_entry_get_addr(this,ANNA_MID_BUFFER_CAPACITY);
}

void anna_buffer_set_capacity(anna_object_t *this, size_t sz)
{
    unsigned char *ptr = anna_buffer_get_payload(this);
    ptr = realloc(ptr, sizeof(char)*sz);
    if(!ptr)
    {
	CRASH;
    }    
    (*(size_t *)anna_entry_get_addr(this,ANNA_MID_BUFFER_CAPACITY)) = sz;
    *(unsigned char **)anna_entry_get_addr(this,ANNA_MID_BUFFER_PAYLOAD) = ptr;
}

unsigned char *anna_buffer_get_payload(anna_object_t *this)
{
    return *(unsigned char **)anna_entry_get_addr(this,ANNA_MID_BUFFER_PAYLOAD);
}

ANNA_VM_NATIVE(anna_buffer_set_int, 3)
{
    ANNA_ENTRY_NULL_CHECK(param[1]);
    anna_buffer_set(anna_as_obj(param[0]), anna_as_int(param[1]), anna_as_int(param[2]));
    return param[2];
}

ANNA_VM_NATIVE(anna_buffer_get_int, 2)
{ 
    ANNA_ENTRY_NULL_CHECK(param[1]);
    return anna_from_int(anna_buffer_get(anna_as_obj(param[0]), anna_as_int(param[1])));
}

ANNA_VM_NATIVE(anna_buffer_get_count_method, 1)
{
    return anna_from_int(anna_buffer_get_count(anna_as_obj(param[0])));
}

ANNA_VM_NATIVE(anna_buffer_get_first, 1)
{
    return anna_from_int(anna_buffer_get(anna_as_obj(param[0]), 0));
}

ANNA_VM_NATIVE(anna_buffer_get_last, 1)
{
    return anna_from_int(anna_buffer_get(anna_as_obj(param[0]), -1));
}

ANNA_VM_NATIVE(anna_buffer_set_count_method, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[1]);
    int sz = anna_as_int(param[1]);
    anna_buffer_set_count(anna_as_obj(param[0]), sz);
    return param[1];
}

ANNA_VM_NATIVE(anna_buffer_init, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    (*anna_entry_get_addr(this,ANNA_MID_BUFFER_PAYLOAD))=0;
    (*(size_t *)anna_entry_get_addr(this,ANNA_MID_BUFFER_CAPACITY)) = 0;    
    (*(size_t *)anna_entry_get_addr(this,ANNA_MID_BUFFER_SIZE)) = 0;
    return param[0];
}

ANNA_VM_NATIVE(anna_buffer_del, 1)
{
    free((*anna_entry_get_addr(anna_as_obj_fast(param[0]),ANNA_MID_BUFFER_PAYLOAD)));
    return param[0];
}

ANNA_VM_NATIVE(anna_buffer_encode, 1)
{    
    anna_object_t *this = anna_as_obj(param[0]);
    anna_object_t *str = anna_string_create(0, 0);
    int i=0;
    unsigned char *src = anna_buffer_get_payload(this);
    size_t count = anna_buffer_get_count(this);
    while(i < count)
    {
	if(!src[i])
	{
	    anna_string_append_cstring(str, 1, L"\0");
	    i++;
	}
	else
	{
	    wchar_t dst;
	    int res = mbtowc(&dst, (char *)&src[i], count-i);
	    switch(res)
	    {
		case -1:
		{
		    return null_entry;
		}
		default:
		{
		    i += res;
		    anna_string_append_cstring(str, 1, &dst);
		}		
	    }
	}
    }
    
    return anna_from_obj(str);
}

ANNA_VM_NATIVE(anna_buffer_decode, 2)
{
    anna_object_t *this = anna_as_obj(param[0]);
    anna_object_t *str = anna_as_obj(param[1]);
    
    if(str == null_object)
    {
	return null_entry;
    }
    
    wchar_t *src = anna_string_payload(str);
    size_t count = anna_string_get_count(str);
    
    int i=0;

    unsigned char *dest = anna_buffer_get_payload(this);
    size_t off = 0;
    size_t dest_count = anna_buffer_get_capacity(this);

    for(i=0; i<count; i++)
    {
	if(dest_count - off < 6)
	{
	    dest_count = maxi(128, 2*dest_count);
	    anna_buffer_set_capacity(this, dest_count);
	    dest = anna_buffer_get_payload(this);
	}
	if(src[i])
	{
	    int res = wctomb((char *)&dest[off], src[i]);
	    if(res == -1)
	    {
		return null_entry;
	    }
	    off += res;
	}
	else
	{
	    dest[off++] = 0;
	}
	
    }
    anna_buffer_set_count(this, off);
    return anna_from_obj(this);
}

void anna_buffer_type_create()
{
    anna_type_t *type = buffer_type;

    mid_t mmid;
    anna_function_t *fun;

    anna_member_create(
	type, ANNA_MID_BUFFER_PAYLOAD, 0, null_type);

    anna_member_create(
	type,
	ANNA_MID_BUFFER_SIZE,
	0,
	null_type);

    anna_member_create(
	type,
	ANNA_MID_BUFFER_CAPACITY,
	0,
	null_type);

     anna_type_t *a_argv[] = 
	{
	    type
	}
    ;
    
    wchar_t *a_argn[]=
	{
	    L"this"
	}
    ;

    anna_member_create_native_method(
	type, ANNA_MID_INIT_PAYLOAD,
	0, &anna_buffer_init,
	type, 1, a_argv, a_argn);
    
    anna_member_create_native_method(
	type, ANNA_MID_DEL, 0, &anna_buffer_del,
	object_type, 1, a_argv, a_argn);    
    
    anna_type_t *i_argv[] = 
	{
	    type,
	    int_type,
	    int_type
	}
    ;

    wchar_t *i_argn[]=
	{
	    L"this", L"index", L"value"
	}
    ;

    mmid = anna_member_create_native_method(
	type,
	anna_mid_get(L"__get__Int"), 0,
	&anna_buffer_get_int, int_type, 2,
	i_argv, i_argn);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(type, mmid)));
    anna_function_alias_add(fun, L"__get__");

    anna_member_create_native_property(
	type, anna_mid_get(L"count"), int_type,
	&anna_buffer_get_count_method,
	&anna_buffer_set_count_method,
	L"The number of bytes in this buffer.");

    anna_member_create_native_property(
	type,
	anna_mid_get(L"first"),
	int_type,
	&anna_buffer_get_first, 0,
	L"The first byte in this buffer.");

    anna_member_create_native_property(
	type,
	anna_mid_get(L"last"),
	int_type,
	&anna_buffer_get_last,
	0,
	L"The last byte in this buffer.");

    mmid = anna_member_create_native_method(
	type,
	anna_mid_get(L"__set__Int"), 0,
	&anna_buffer_set_int, int_type, 3,
	i_argv, i_argn);    
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(type, mmid)));
    anna_function_alias_add(fun, L"__set__");

    anna_member_create_native_method(
	type, anna_mid_get(L"encode"), 0,
	&anna_buffer_encode, string_type, 1,
	a_argv, a_argn);
    anna_member_document(
	type, anna_mid_get(L"encode"), 
	anna_intern_static(L"Encode the byte array into a character string using the default encoding of the locale."));
    
    anna_type_t *d_argv[] = 
	{
	    type,
	    string_type
	}
    ;
    
    wchar_t *d_argn[]=
	{
	    L"this",
	    L"value"
	}
    ;
    
    anna_member_create_native_method(
	type, anna_mid_get(L"decode"), 0,
	&anna_buffer_decode, type, 2,
	d_argv, d_argn);
    anna_member_document(
	type, anna_mid_get(L"decode"), 
	anna_intern_static(L"Decode the String into a byte array using the default encoding of the locale."));
    
}

