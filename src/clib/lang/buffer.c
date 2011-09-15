
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

ANNA_VM_NATIVE(anna_buffer_del, 1)
{
    free((*anna_entry_get_addr(anna_as_obj_fast(param[0]),ANNA_MID_BUFFER_PAYLOAD)));
    return param[0];
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
    *(size_t *)anna_entry_get_addr(this,ANNA_MID_BUFFER_SIZE) = off;
    return anna_from_obj(this);
}

void anna_buffer_type_create()
{
    anna_type_t *type = buffer_type;

    mid_t mmid;
    anna_function_t *fun;


    anna_type_document(
	type,
	L"The buffer type represents a byte oriented mutable array of binary data. It is primarily used for reading and writing from file descriptors.");  

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
	type, 1, a_argv, a_argn, 0, 0);
    
    anna_member_create_native_method(
	type, ANNA_MID_DEL, 0, &anna_buffer_del,
	object_type, 1, a_argv, a_argn, 0, 0);
    
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
	anna_mid_get(L"__get__Int__"), 0,
	&anna_buffer_get_int, int_type, 2,
	i_argv, i_argn, 0, 0);
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
	anna_mid_get(L"__set__Int__"), 0,
	&anna_buffer_set_int, int_type, 3,
	i_argv, i_argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(type, mmid)));
    anna_function_alias_add(fun, L"__set__");

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
    
}

