#include "autogen/float_i.c"

static void anna_float_set(anna_object_t *this, double value)
{
    memcpy(anna_entry_get_addr(this,ANNA_MID_FLOAT_PAYLOAD), &value, sizeof(double));
}

anna_object_t *anna_float_create(double value)
{
    anna_object_t *obj= anna_object_create(float_type);
    anna_float_set(obj, value);
    return obj;
}

double anna_float_get(anna_object_t *this)
{
    double result;
    memcpy(&result, anna_entry_get_addr(this,ANNA_MID_FLOAT_PAYLOAD), sizeof(double));
    return result;
}

ANNA_VM_NATIVE(anna_float_cmp, 2)
{
    if(unlikely( anna_entry_null(param[1])))
    {
        return null_entry;
    }
    double v1 = anna_as_float(param[0]);
    double v2 = anna_as_float(param[1]);
    if(v1 > v2)
    {
	return anna_from_int(1);
    }
    else if(v1 < v2)
    {
	return anna_from_int(-1);
    }
    else{
	return anna_from_int(0);
    }   
}

ANNA_VM_NATIVE(anna_float_cmp_int, 2)
{
    if(unlikely( anna_entry_null(param[1])))
    {
        return null_entry;
    }

    double v1 = anna_as_float(param[0]);
    double v2 = (double)anna_as_int(param[1]);
    if(v1 > v2)
    {
	return anna_from_int(1);
    }
    else if(v1 < v2)
    {
	return anna_from_int(-1);
    }
    else{
	return anna_from_int(0);
    }   
    
}

ANNA_VM_NATIVE(anna_float_cmp_int_reverse, 2)
{
    if(unlikely( anna_entry_null(param[1])))
    {
        return null_entry;
    }

    double v2 = anna_as_float(param[0]);
    double v1 = (double)anna_as_int(param[1]);
    if(v1 > v2)
    {
	return anna_from_int(1);
    }
    else if(v1 < v2)
    {
	return anna_from_int(-1);
    }
    else{
	return anna_from_int(0);
    }   
    
}

ANNA_VM_NATIVE(anna_float_to_string, 1)
{
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"%f", anna_as_float(param[0]));
    wchar_t *buff = sb_content(&sb);
    wchar_t *comma = wcschr(buff, ',');
    if(comma)
    {
	*comma = '.';
    }
    anna_entry_t res = anna_from_obj(anna_string_create(sb_count(&sb), buff));
    sb_destroy(&sb);
    return res;
}

ANNA_VM_NATIVE(anna_float_hash, 1)
{
    double cmp = anna_as_float(param[0]);
    int res = anna_hash((int *)&cmp, sizeof(double) / sizeof(int));
    return anna_from_int(res);
}

ANNA_VM_NATIVE(anna_float_convert_string, 1)
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
    
    /* Strip any underscores from the string */
    wchar_t *in = str;
    wchar_t *out = str;
    while(*in)
    {
	if(*in == L'_')
	{
	    in++;
	}
	else
	{
	    *out++ = *in++;
	}
    }
    *out = 0;
    
    /* Convert to narrow string and send if to strtod */
    char *nstr = wcs2str(str);
    char *end=0;

    int olderr = errno;
    errno=0;    
    double res = strtod(nstr, &end);
    int newerr = errno;
    errno=olderr;
    int end_not_reached = (*end != 0);
    free(str); 
    free(nstr);

    if(newerr || end_not_reached)
    {
	return null_entry;
    }
    return anna_from_float(res);
}

ANNA_VM_NATIVE(anna_float_convert_float, 1)
{
    return param[0];
}

ANNA_VM_NATIVE(anna_float_convert_int, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    mpz_t *int_val = anna_int_unwrap(anna_as_obj(param[0]));
    double res = mpz_get_d(*int_val);
    return anna_from_float(res);
}

ANNA_VM_NATIVE(anna_float_max_exponent, 1)
{
    return anna_from_int(DBL_MAX_EXP);
}

char *dtoa(
    double d, int mode, int ndigits,
    int *decpt, int *sign, char **rve);
void freedtoa(char *s);


ANNA_VM_NATIVE(anna_float_format, 2)
{
    double val = anna_as_float(param[0]);
    int mode = 0;
    int ndigits = 0;

    if(!anna_entry_null(param[1]))
    {
	ndigits = anna_as_int(param[1]);
	mode = 3;
    }
    
    int decpt;
    int sign;
    char *rve = 0;
    char *res = dtoa(val, mode, ndigits, &decpt, &sign, &rve);
    anna_object_t *str = anna_string_create_narrow(decpt, res);
    if(strlen(res+decpt))
    {
	anna_string_append_cstring(str, L".", 1);
	wchar_t *wide = str2wcs(res+decpt);
	anna_string_append_cstring(str, wide, wcslen(wide));
	free(wide);
    }
    freedtoa(res);
    
    return anna_from_obj(str);
}

void anna_float_type_create()
{
    anna_type_t *argv[] = 
	{
	    float_type, any_type
	}
    ;
    anna_type_t *float_cmp_argv[] = 
	{
	    float_type, float_type
	}
    ;
    anna_type_t *int_cmp_argv[] = 
	{
	    float_type, int_type
	}
    ;
    wchar_t *argn[]=
	{
	    L"this", L"other"
	}
    ;

    mid_t mmid;

    anna_type_make_sendable(float_type);

    anna_type_document(
	float_type,
	L"The float Type represents a double precision floating point number.");
    
    anna_type_document(
	float_type,
	L"Float objects are imutable, can be used as hash keys and implement the basic arithmetic operations like addition, multiplication, etc. Arithmetic operations on combinations of Floats and Ints result in a Float object. Arithmetic operations on Floats ad Complex numbers result in a Complex object.");
    
    
    anna_member_create_blob(
	float_type, ANNA_MID_FLOAT_PAYLOAD, 0,
	sizeof(double));
    
    mmid = anna_member_create_native_method(
	float_type, anna_mid_get(L"cmpFloat"), 0,
	&anna_float_cmp, int_type, 2, float_cmp_argv, argn, 0, 0);
    anna_member_alias(float_type, mmid, L"__cmp__");
    
    mmid = anna_member_create_native_method(
	float_type, anna_mid_get(L"cmpInt"), 0,
	&anna_float_cmp_int, int_type, 2, int_cmp_argv, argn, 0, 0);
    anna_member_alias(float_type, mmid, L"__cmp__");
    
    mmid = anna_member_create_native_method(
	float_type, anna_mid_get(L"cmpIntReverse"), 0,
	&anna_float_cmp_int_reverse, int_type, 2, int_cmp_argv, argn, 0, 0);
    anna_member_alias_reverse(float_type, mmid, L"__cmp__");
    
    anna_member_create_native_method(
	float_type, ANNA_MID_TO_STRING, 0,
	&anna_float_to_string, string_type, 1,
	argv, argn, 0, 0);
    anna_member_create_native_method(
	float_type,
	ANNA_MID_HASH_CODE,
	0,
	&anna_float_hash,
	int_type,
	1,
	argv,
	argn, 0, 0);


    anna_type_t *format_argv[] = 
	{
	    float_type, int_type
	}
    ;
    wchar_t *format_argn[]=
	{
	    L"this", L"digits"
	}
    ;
    anna_node_t *format_argd[] = 
	{
	    0, (anna_node_t *)anna_node_create_null(0)
	}
    ;
    
    anna_member_create_native_method(
	float_type, anna_mid_get(L"format"), 0,
	&anna_float_format, string_type, 2, format_argv, format_argn, format_argd, L"Convert this floating point number to a string");
    
    wchar_t *conv_argn[]=
	{
	    L"value"
	}
    ;

    mmid = anna_member_create_native_type_method(
	float_type, anna_mid_get(L"convertString"),
	0, &anna_float_convert_string,
	float_type, 1, &string_type, conv_argn, 0,
	L"Convert the specified character String to a floating point number.");
    anna_member_alias(float_type, mmid, L"convert");

    mmid = anna_member_create_native_type_method(
	float_type, anna_mid_get(L"convertFloat"),
	0, &anna_float_convert_float,
	float_type, 1, &float_type, conv_argn, 0,
	L"Convert the specified floating point number to a floating point number. (This is a no-op.)");
    anna_member_alias(float_type, mmid, L"convert");

    mmid = anna_member_create_native_type_method(
	float_type, anna_mid_get(L"convertInt"),
	0, &anna_float_convert_int,
	float_type, 1, &int_type, conv_argn, 0,
	L"Convert the specified Integer value to a floating point number.");
    anna_member_alias(float_type, mmid, L"convert");
    
    anna_member_create_native_property(
	float_type, anna_mid_get(L"maxExponent"),
	int_type, &anna_float_max_exponent, 0,
	L"The largest value for the exponent that can be stored in a float.");
    
    anna_float_type_i_create();
}
