
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
//    anna_message(L"Set el %d in buffer of %d elements\n", pos, size);
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
    size_t old_sz = (*(size_t *)anna_entry_get_addr(this,ANNA_MID_BUFFER_SIZE));
    
    if(sz>capacity)
    {
	anna_buffer_set_capacity(this, sz);
    }
    unsigned char *ptr = anna_buffer_get_payload(this);
    if(sz > old_sz)
    {
        memset(ptr+old_sz, 0, sz-old_sz);
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

int anna_buffer_ensure_capacity(anna_object_t *this, size_t sz)
{
    unsigned char *ptr = anna_buffer_get_payload(this);
    size_t old_cap = (*(size_t *)anna_entry_get_addr(this,ANNA_MID_BUFFER_CAPACITY));
    size_t old_sz = (*(size_t *)anna_entry_get_addr(this,ANNA_MID_BUFFER_SIZE));

    if(old_cap >= sz)
    {
        if(old_sz < sz)
	{
	    ptr = anna_buffer_get_payload(this);
	    memset(ptr+old_sz, 0, sz-old_sz);
	    *(size_t *)anna_entry_get_addr(this,ANNA_MID_BUFFER_SIZE) = sz;
	}
	return 0;
    }
    size_t cap = anna_size_round(sz);
    
    ptr = realloc(ptr, sizeof(char)*cap);
    if(!ptr)
    {
	return 1;
    }
    memset(ptr+old_sz, 0, cap-old_sz);
    (*(size_t *)anna_entry_get_addr(this,ANNA_MID_BUFFER_CAPACITY)) = cap;
    (*(size_t *)anna_entry_get_addr(this,ANNA_MID_BUFFER_SIZE)) = maxi(sz, old_sz);
    *(unsigned char **)anna_entry_get_addr(this,ANNA_MID_BUFFER_PAYLOAD) = ptr;
    return 0;
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

static void anna_buffer_del(anna_object_t *victim)
{
    free(anna_entry_get(victim, ANNA_MID_BUFFER_PAYLOAD));
}

ANNA_VM_NATIVE(anna_buffer_encode, 2)
{    
    anna_object_t *this = anna_as_obj(param[0]);
    int null_terminated = param[1] != null_entry;
    anna_object_t *str = anna_string_create(0, 0);
    int i=0;
    unsigned char *src = anna_buffer_get_payload(this);
    size_t count = anna_buffer_get_count(this);
    while(i < count)
    {
	if(!src[i])
	{
	    if(null_terminated)
	    {
		break;
	    }
	    
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
    free(src);
    *(size_t *)anna_entry_get_addr(this,ANNA_MID_BUFFER_SIZE) = off;
    return anna_from_obj(this);
}

ANNA_VM_NATIVE(anna_buffer_to_string, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *buffer = anna_as_obj(param[0]);
    anna_object_t *res = anna_string_create(0, L"");
    size_t sz =  anna_buffer_get_count(buffer);
    unsigned char *ptr = anna_buffer_get_payload(buffer);
    size_t i;
    wchar_t *hex = L"0123456789abcdef";
    
    for(i=0; i<sz; i++)
    {
	char ch = ptr[i];
	anna_string_append_cstring(res, 1, &hex[ch /16]);
	anna_string_append_cstring(res, 1, &hex[ch % 16]);
    }
    return anna_from_obj(res);
}

static void anna_buffer_iterator_update(anna_object_t *iter, int off)
{
    anna_object_t *buffer = anna_as_obj(anna_entry_get(iter, ANNA_MID_COLLECTION));
    
    if((off >= 0) && (off < anna_buffer_get_count(buffer)))
    {
	anna_entry_set(iter, ANNA_MID_VALUE, anna_from_int(anna_buffer_get(buffer, off)));
	anna_entry_set(iter, ANNA_MID_VALID, anna_from_int(1));
    }
    else
    {
	anna_entry_set(iter, ANNA_MID_VALUE, null_entry);
	anna_entry_set(iter, ANNA_MID_VALID, null_entry);
    }
    anna_entry_set(iter, ANNA_MID_KEY, anna_from_int(off));
}

ANNA_VM_NATIVE(anna_buffer_get_iterator, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *buffer = anna_as_obj(param[0]);
    anna_object_t *iter = anna_object_create(
	anna_type_unwrap((anna_object_t *)anna_entry_get_static(buffer->type, ANNA_MID_ITERATOR_TYPE)));
    anna_entry_set(iter, ANNA_MID_COLLECTION, param[0]);
    anna_buffer_iterator_update(iter, 0);
    return anna_from_obj(iter);
}

ANNA_VM_NATIVE(anna_buffer_iterator_next, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *iter = anna_as_obj(param[0]);
    anna_buffer_iterator_update(iter, anna_as_int(anna_entry_get(iter, ANNA_MID_KEY))+1);
    return param[0];
}

ANNA_VM_NATIVE(anna_buffer_iterator_get_value, 1)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    anna_object_t *iter = anna_as_obj(param[0]);
    anna_object_t *buffer = anna_as_obj(anna_entry_get(iter, ANNA_MID_COLLECTION));
    int off = anna_as_int(anna_entry_get(iter, ANNA_MID_KEY));
    
    if((off >= 0) && (off < anna_buffer_get_count(buffer)))
    {
	unsigned char *ptr = anna_buffer_get_payload(buffer);
	return anna_from_int(ptr[off]);
    }
    else
    {
	return null_entry;
    }
}

ANNA_VM_NATIVE(anna_buffer_iterator_set_value, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    ANNA_ENTRY_NULL_CHECK(param[1]);

    anna_object_t *iter = anna_as_obj(param[0]);
    anna_object_t *buffer = anna_as_obj(anna_entry_get(iter, ANNA_MID_COLLECTION));
    int off = anna_as_int(anna_entry_get(iter, ANNA_MID_KEY));

    if((off >= 0) && (off < anna_buffer_get_count(buffer)))
    {
	unsigned char *ptr = anna_buffer_get_payload(buffer);
	ptr[off] = anna_as_int(param[1]);
    }
    return param[1];
}

static anna_type_t *anna_buffer_iterator_create(
    anna_type_t *type)
{
    anna_type_t *iter = anna_type_create(L"Iterator", 0);
    anna_member_create(
	iter, ANNA_MID_COLLECTION, ANNA_MEMBER_IMUTABLE, type);    
    anna_member_create(
	iter, ANNA_MID_KEY, ANNA_MEMBER_IMUTABLE, int_type);

    anna_member_create_native_property(
	iter, ANNA_MID_VALUE, int_type,
	&anna_buffer_iterator_get_value,
	&anna_buffer_iterator_set_value,
	0);
    
    anna_member_create(
	iter, ANNA_MID_VALID, ANNA_MEMBER_IMUTABLE, object_type);
    anna_type_copy_object(iter);
    
    anna_type_t *iter_argv[] = 
	{
	    iter
	}
    ;
    
    wchar_t *iter_argn[]=
	{
	    L"this"
	}
    ;

    anna_member_create_native_method(
	iter,
	ANNA_MID_NEXT_ASSIGN, 0,
	&anna_buffer_iterator_next, iter, 1,
	iter_argv, iter_argn, 0, 0);

    anna_util_iterator_iterator(iter);
        
    anna_type_close(iter);

    return iter;
}

void anna_buffer_type_create()
{
    anna_type_t *type = buffer_type;
    mid_t mmid;

    anna_type_document(
	type,
	L"The buffer type represents a byte oriented mutable array of binary data. It is primarily used for reading and writing from file descriptors, but can also be used to encode text strings to a specific encoding and various other low level tasks.");  

    anna_type_document(
	type,
	L"Buffers are in many ways similar to a list of Int objects, except that the values have a range of 0 to 255 and can not be null. These differences make it possible to make the Buffer type significantly more efficient than a List, in addition to making them suitable for low level calls like unix.io.read and unix.io.write.");  

    anna_type_document(
	type,
	L"All buffers are read-write, though it is possible that a future Anna version will provide read only buffers.");  

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

    anna_member_create(
	type,
	ANNA_MID_ITERATOR_TYPE,
	ANNA_MEMBER_STATIC,
	type_type);

    anna_type_t *iter = anna_buffer_iterator_create(type);

    anna_entry_set_static(
	type, ANNA_MID_ITERATOR_TYPE, 
	anna_from_obj(anna_type_wrap(iter)));
    anna_member_create_native_property(
	type, ANNA_MID_ITERATOR, iter,
	&anna_buffer_get_iterator, 0, 0);

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
	type, 1, a_argv, a_argn, 0, 0);
    
    anna_type_finalizer_add(
	type, anna_buffer_del);
    
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
	anna_mid_get(L"get"), 0,
	&anna_buffer_get_int, int_type, 2,
	i_argv, i_argn, 0, 0);
    anna_member_alias(type, mmid, L"__get__");

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
	anna_mid_get(L"set"), 0,
	&anna_buffer_set_int, int_type, 3,
	i_argv, i_argn, 0, 0);
    anna_member_alias(type, mmid, L"__set__");

    anna_type_t *e_argv[] = 
	{
	    type,
	    int_type
	}
    ;
    
    wchar_t *e_argn[]=
	{
	    L"this",
	    L"nullTerminated"
	}
    ;
    
    anna_node_t *e_argd[] = 
	{
	    0, anna_node_create_null(0)
	}
    ;

    mmid = anna_member_create_native_method(
	type, anna_mid_get(L"encode"), 0,
	&anna_buffer_encode, string_type, 2,
	e_argv, e_argn, e_argd, L"Encode the byte array into a character string using the default encoding of the locale.");
    
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
	d_argv, d_argn, 0, L"Decode the String into a byte array using the default encoding of the locale.");

    anna_member_create_native_method(
	type, ANNA_MID_TO_STRING, 0,
	&anna_buffer_to_string, string_type, 1,
	i_argv, i_argn, 0, L"Returns a hexadecimal string representation of the contents of this buffer.");    
    
}

