
#include "autogen/int_i.c"

static void *anna_int_alloc(size_t sz)
{
    anna_alloc_data()->count += sz;
    return malloc(sz);
}

static void *anna_int_realloc(void *ptr, size_t osz, size_t nsz)
{
    anna_alloc_data()->count += nsz;
    anna_alloc_data()->count -= osz;
    return realloc(ptr, nsz);
}

static void anna_int_free(void *ptr, size_t sz)
{
    anna_alloc_data()->count -= sz;
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

int anna_is_int(anna_entry_t this)
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

//    anna_message(L"Create bignum %s from bignum %s\n", mpz_get_str(0, 10, *(mpz_t *)anna_entry_get_addr(obj,ANNA_MID_INT_PAYLOAD)), mpz_get_str(0, 10, value));
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

anna_entry_t anna_int_entry(anna_object_t *this)
{
    mpz_t *me = anna_int_unwrap(this);
    if(mpz_sizeinbase(*me, 2)<=ANNA_SMALL_MAX_BIT)
    {
//	anna_message(L"Weee, small int %d (%d bits)\n", anna_int_get(this), mpz_sizeinbase(*me, 2));
	return anna_from_int(anna_int_get(this));
    }
//    anna_message(L"Boo, large int (%d bits)\n", mpz_sizeinbase(*me, 2));
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
    anna_entry_t res = anna_from_obj(anna_string_create(sb_count(&sb), sb_content(&sb)));
    sb_destroy(&sb);
    return res;
}

static void anna_int_del(anna_object_t *victim)
{
    mpz_clear(*anna_int_unwrap(victim));
}

ANNA_VM_NATIVE(anna_int_convert_string, 2)
{
    ANNA_ENTRY_NULL_CHECK(param[0]);
    ANNA_ENTRY_NULL_CHECK(param[1]);

    int base = anna_as_int(param[1]);
    if(base < 2 || base > 36)
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
    mpz_set_si(mpbase, base);
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
	else if( (ch >= 'a') && (ch <= 'z'))
	{
	    val = ch - 'a' + 10;
	}
	else if( (ch >= 'A') && (ch <= 'Z'))
	{
	    val = ch - 'A' + 10;
	}	
	else
	{
	    break;
	}

	if(val >= base)
	{
	    return null_entry;
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
    anna_type_t *i_cmp_argv[] = 
	{
	    int_type, int_type
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
    
    anna_type_t *conv_argv[] = 
	{
	    string_type, int_type
	}
    ;

    wchar_t *conv_argn[]=
	{
	    L"value", L"base"
	}
    ;

    mpz_t mp;
    mpz_init(mp);
    mpz_set_si(mp, 10);
    
    anna_node_t *conv_argd[]=
	{
	    0, (anna_node_t *)anna_node_create_int_literal(0, mp)
	}
    ;
    mpz_clear(mp);
    
    mid_t mmid;

    anna_type_make_sendable(int_type);

    anna_type_document(
	int_type,
	L"The Int type is integer type of the Anna language.");
    
    anna_type_document(
	int_type,
	L"Anna Int objects are imutable, meaning their value never changes. Integers can be used as hash keys and implement normal mathematical operations like addition (using the + operator), multiplication (using the * operator) and so on.");
    
    anna_type_document(
	int_type,
	L"Any arithmetic operation involving only Int objects will result in a new Int. Specifically, dividing one Int with another will result in a new Int object, which is the truncated value of the division. Convert one of the numbers to a Float using code like <code class='anna-code'>Float::convert(someIntValue)</code> if floating point division is desired.");

    anna_type_document(
	int_type,
	L"Anna Int objects are arbitrary precision, i.e. they never overflow. Small integer values, numbers that use 30 bits or less to represent (including the sign bit) are usually stored directly on the stack and use no heap memory at all. Larger integers are implemented using an arbitrary precision library. Aside from resulting in lower memory useage, this optimization is completely transparent - Int objects behave like all other Anna objects in every way, including the fact that they can be inherited from.");
    
    anna_member_create_blob(int_type, ANNA_MID_INT_PAYLOAD, 0, sizeof(mpz_t));
    
    anna_member_create_native_method(
	int_type, anna_mid_get(L"__init__"), 0,
	&anna_int_init_i, object_type, 2, ii_argv,
	ii_argn, 0, L"Create a new Int object with the same value as the specified Int object.");
    
    anna_member_create_native_method(
	int_type,
	anna_mid_get(L"__cmp__"),
	0,
	&anna_int_cmp,
	int_type,
	2,
	i_cmp_argv,
	i_argn, 0, 
	L"Compare this Int to another object. Returns null if the other object is not an Int. Return a negative number, zero or a positive number if the other Int is smaller than, equal to or greater than this Int, respectively.");
    
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
	2, conv_argv, conv_argn, conv_argd, 
	L"Convert a String to an Int. The String must be in the specified base (10 by default) and the entire string (not just a prefix of it) must be a legal number. The base must be between2 and 36.");
    anna_member_alias(int_type, mmid, L"convert");
    
    mmid = anna_member_create_native_type_method(
	int_type, anna_mid_get(L"convertFloat"),
	0, &anna_int_convert_float, int_type, 1,
	&float_type, conv_argn, 0,
	L"Convert a Float to an Int, truncating any stray precision.");
    anna_member_alias(int_type, mmid, L"convert");

    mmid = anna_member_create_native_type_method(
	int_type, anna_mid_get(L"convertChar"), 0,
	&anna_int_convert_char, int_type, 1, &char_type,
	conv_argn, 0,
	L"Returns the ordinal number of a Char. This method returns the same value as the Char.ordinal property.");
    anna_member_alias(int_type, mmid, L"convert");

    mmid = anna_member_create_native_type_method(
	int_type, anna_mid_get(L"convertInt"), 0,
	&anna_int_convert_int, int_type, 1, &int_type,
	conv_argn, 0,
	L"Converts an Int to itself. This is a noop.");
    anna_member_alias(int_type, mmid, L"convert");

    anna_type_finalizer_add(
	int_type, anna_int_del);

    anna_int_type_i_create();


    anna_member_document(
	int_type,
	anna_mid_get(L"mod"),
	L"Calculate the modulus of the two numbers.");

    anna_member_document(
	int_type,
	anna_mid_get(L"shl"),
	L"Left shift the binary representation of this integer the specified number of steps.");

    anna_member_document(
	int_type,
	anna_mid_get(L"shr"),
	L"Right shift the binary representation of this integer the specified number of steps.");

    anna_member_document(
	int_type,
	anna_mid_get(L"bitand"),
	L"Returns the bitwise and of the two numbers.");

    anna_member_document(
	int_type,
	anna_mid_get(L"bitor"),
	L"Returns the bitwise or of the two numbers.");

    anna_member_document(
	int_type,
	anna_mid_get(L"bitxor"),
	L"Returns the bitwise xor of the two numbers.");

}
