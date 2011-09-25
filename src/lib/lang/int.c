
#include "autogen/anna_int_i.c"

static void *anna_int_alloc(size_t sz)
{
    anna_alloc_count += sz;
    return malloc(sz);
}

static void *anna_int_realloc(void *ptr, size_t osz, size_t nsz)
{
    anna_alloc_count += nsz;
    anna_alloc_count -= osz;
    return realloc(ptr, nsz);
}

static void anna_int_free(void *ptr, size_t sz)
{
    anna_alloc_count -= sz;
    free(ptr);
}

void anna_int_init(void)
{
    mp_set_memory_functions (
	&anna_int_alloc,
	&anna_int_realloc,
	&anna_int_free);
}

static void anna_int_set(anna_object_t *this, long value)
{
    mpz_init(*(mpz_t *)anna_entry_get_addr(this,ANNA_MID_INT_PAYLOAD));
    mpz_set_si(*(mpz_t *)anna_entry_get_addr(this,ANNA_MID_INT_PAYLOAD), value);
}

int anna_is_int(anna_entry_t *this)
{
    if(anna_is_int_small(this))
    {
	return 1;
    }
    else if(anna_is_obj(this))
    {
	anna_object_t *obj = anna_as_obj_fast(this);
	return !!obj->type->mid_identifier[ANNA_MID_INT_PAYLOAD];
    }
    return 0;    
}


anna_object_t *anna_int_create_mp(mpz_t value)
{
    anna_object_t *obj= anna_object_create(int_type);
    mpz_init(*(mpz_t *)anna_entry_get_addr(obj,ANNA_MID_INT_PAYLOAD));
    mpz_set(*(mpz_t *)anna_entry_get_addr(obj,ANNA_MID_INT_PAYLOAD), value);

//    wprintf(L"Create bignum %s from bignum %s\n", mpz_get_str(0, 10, *(mpz_t *)anna_entry_get_addr(obj,ANNA_MID_INT_PAYLOAD)), mpz_get_str(0, 10, value));
    return obj;
}

anna_object_t *anna_int_create(long value)
{
    anna_object_t *obj= anna_object_create(int_type);
    anna_int_set(obj, value);
    return obj;
}

anna_object_t *anna_int_create_ll(long long value)
{
    anna_object_t *obj= anna_object_create(int_type);
    mpz_init(*(mpz_t *)anna_entry_get_addr(obj,ANNA_MID_INT_PAYLOAD));
    mpz_set_si(*(mpz_t *)anna_entry_get_addr(obj,ANNA_MID_INT_PAYLOAD), value>>32);
    return obj;
}

mpz_t *anna_int_unwrap(anna_object_t *this)
{
    return (mpz_t *)anna_entry_get_addr(this,ANNA_MID_INT_PAYLOAD);
}

long int anna_int_get(anna_object_t *this)
{
    return mpz_get_si(*(mpz_t *)anna_entry_get_addr(this,ANNA_MID_INT_PAYLOAD));
}

anna_entry_t *anna_int_entry(anna_object_t *this)
{
    mpz_t *me = anna_int_unwrap(this);
    if(mpz_sizeinbase(*me, 2)<=ANNA_SMALL_MAX_BIT)
    {
//	wprintf(L"Weee, small int %d (%d bits)\n", anna_int_get(this), mpz_sizeinbase(*me, 2));
	return anna_from_int(anna_int_get(this));
    }
    return anna_from_obj(this);
}

ANNA_VM_NATIVE(anna_int_init_i, 2)
{
    anna_int_set(anna_as_obj(param[0]), anna_as_int(param[1]));
    return param[0];
}

ANNA_VM_NATIVE(anna_int_hash, 1)
{
    return anna_from_int(
	mpz_get_si(
	    *anna_int_unwrap(anna_as_obj(param[0]))) & ANNA_INT_FAST_MAX);
}

ANNA_VM_NATIVE(anna_int_cmp, 2)
{
    if(unlikely(anna_entry_null(param[1])))
    {
	return null_entry;
    }
    else if(anna_is_int_small(param[1]))
    {
	return anna_from_int(
	    (long)mpz_cmp_si(
		*anna_int_unwrap(anna_as_obj(param[0])), 
		anna_as_int(param[1])));
    }
    else if(anna_is_int(param[1]))
    {
	return anna_from_int(
	    (long)mpz_cmp(
		*anna_int_unwrap(anna_as_obj(param[0])), 
		*anna_int_unwrap(anna_as_obj_fast(param[1]))));
    }
    else
    {
	return null_entry;	    
    }
}

ANNA_VM_NATIVE(anna_int_to_string, 1)
{
    char *nstr = mpz_get_str(0, 10, *anna_int_unwrap(anna_as_obj(param[0])));
    
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"%s", nstr);

    free(nstr);
    anna_entry_t *res = anna_from_obj(anna_string_create(sb_length(&sb), sb_content(&sb)));
    sb_destroy(&sb);
    return res;
}

static void anna_int_del(anna_object_t *victim)
{
    mpz_clear(*anna_int_unwrap(victim));
}

ANNA_VM_NATIVE(anna_int_convert_string, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    wchar_t *str = anna_string_payload(anna_as_obj(param[0]));
    if(wcslen(str) != anna_string_get_count(anna_as_obj(param[0])))
    {
	return null_entry;	
    }
    
    wchar_t *c = str;
    int sign = 1;
    
    if(*c == '-')
    {
	c++;
	sign = -1;
    }
    
    
    mpz_t res;
    mpz_t mpval;
    mpz_t mpbase;
    mpz_init(res);
    mpz_init(mpval);
    mpz_init(mpbase);
    mpz_set_si(res, 0);
    mpz_set_si(mpbase, 10);
    wchar_t ch;
    
    while(1)
    {
	ch = *(c++);
	if(ch == '_')
	    continue;

	int val;

	if( (ch >= '0') && (ch <= '9'))
	{
	    val = ch - '0';
	}
	else
	{
	    break;
	}

	mpz_set_si(mpval, val);

	mpz_mul(res, mpbase, res);
	mpz_add(res, res, mpval);
    }
    mpz_mul_si(res, res, sign);
    

    anna_object_t *res_obj = anna_int_create_mp(res);
    int err = !!ch;
    free(str);
    mpz_clear(res);
    mpz_clear(mpbase);
    mpz_clear(mpval);

    if(err)
    {
	return null_entry;
    }

    return anna_from_obj(res_obj);
}

ANNA_VM_NATIVE(anna_int_convert_float, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    double value = anna_as_float(param[0]);
    
    mpz_t res;
    mpz_init(res);
    mpz_set_d(res, value);
        
    anna_object_t *res_obj = anna_int_create_mp(res);
    mpz_clear(res);

    return anna_from_obj(res_obj);
}

ANNA_VM_NATIVE(anna_int_convert_int, 1)
{
    return param[0];
}

ANNA_VM_NATIVE(anna_int_convert_char, 1)
{
    return anna_from_int((int)anna_as_char(param[0]));
}

void anna_int_type_create()
{
    anna_type_t *i_argv[] = 
	{
	    int_type, object_type
	}
    ;
    wchar_t *i_argn[]=
	{
	    L"this", L"other"
	}
    ;
    
    anna_type_t *ii_argv[] = 
	{
	    int_type, int_type
	}
    ;
    wchar_t *ii_argn[]=
	{
	    L"this", L"other"
	}
    ;
    
    wchar_t *conv_argn[]=
	{
	    L"value"
	}
    ;

    mid_t mmid;
    anna_function_t *fun;

    anna_type_document(
	int_type,
	L"The Int type is the basic integer type of the Anna language.");
    
    anna_type_document(
	int_type,
	L"Anna Int objects are arbitrary precision, i.e. they never overflow. Small integer values, numbers that use 29 bits or less to represent (excluding the sign bit, so 30 bits total of data) can be stored directly on the stack and use no heap memory at all. Larger integers are implemented using a multiple precision library such as GNU MP.");    

    anna_type_document(
	int_type,
	L"Anna Int objects are imutable, meaning their value never changes.");

    anna_member_create_blob(int_type, ANNA_MID_INT_PAYLOAD, 0, sizeof(mpz_t));
    
    anna_member_create_native_method(
	int_type, anna_mid_get(L"__init__"), 0,
	&anna_int_init_i, object_type, 2, ii_argv,
	ii_argn, 0, 0);
    
    anna_member_create_native_method(
	int_type,
	anna_mid_get(L"__cmp__"),
	0,
	&anna_int_cmp,
	int_type,
	2,
	i_argv,
	i_argn, 0, 0);
    
    anna_member_create_native_method(
	int_type, ANNA_MID_HASH_CODE, 0,
	&anna_int_hash, int_type, 1, i_argv,
	i_argn, 0, 0);

    anna_member_create_native_method(
	int_type,
	ANNA_MID_TO_STRING,
	0,
	&anna_int_to_string,
	string_type,
	1,
	i_argv,
	i_argn, 0, 0);

    mmid = anna_member_create_native_type_method(
	int_type, anna_mid_get(L"convertString"),
	0, &anna_int_convert_string, int_type,
	1, &string_type, conv_argn, 0, 
	L"Convert a String to an Int. The String must be in decimal and the entire string (not just a prefix of it) must be a legal number.");
    fun = anna_function_unwrap(
	anna_as_obj_fast(anna_entry_get_static(int_type, mmid)));
    anna_function_alias_add(fun, L"convert");
    
    mmid = anna_member_create_native_type_method(
	int_type, anna_mid_get(L"convertFloat"),
	0, &anna_int_convert_float, int_type, 1,
	&float_type, conv_argn, 0,
	L"Convert a Float to an Int, truncating any stray precision.");
    fun = anna_function_unwrap(
	anna_as_obj_fast(anna_entry_get_static(int_type, mmid)));
    anna_function_alias_add(fun, L"convert");

    mmid = anna_member_create_native_type_method(
	int_type, anna_mid_get(L"convertChar"), 0,
	&anna_int_convert_char, int_type, 1, &char_type,
	conv_argn, 0,
	L"Returns the ordinal number of a Char. This method returns the same value as the Char.ordinal property.");
    fun = anna_function_unwrap(
	anna_as_obj_fast(anna_entry_get_static(int_type, mmid)));
    anna_function_alias_add(fun, L"convert");

    mmid = anna_member_create_native_type_method(
	int_type, anna_mid_get(L"convertInt"), 0,
	&anna_int_convert_int, int_type, 1, &int_type,
	conv_argn, 0,
	L"Converts an Int to itself. This is a noop.");
    fun = anna_function_unwrap(
	anna_as_obj_fast(anna_entry_get_static(int_type, mmid)));
    anna_function_alias_add(fun, L"convert");

    anna_type_finalizer_add(
	int_type, anna_int_del);

    anna_int_type_i_create();
}
