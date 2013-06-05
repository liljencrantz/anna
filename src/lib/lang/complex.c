//ROOT: src/lib/lang/lang.c

#include "autogen/complex_i.c"

static inline void anna_complex_set(anna_object_t *this, complex double value)
{
    size_t off = this->type->mid_identifier[ANNA_MID_COMPLEX_PAYLOAD]->offset;
    memcpy(&this->member[off], &value, sizeof(complex double));
}

int anna_is_complex(anna_entry_t this)
{
    anna_object_t *obj = anna_as_obj(this);
    return !!obj->type->mid_identifier[ANNA_MID_COMPLEX_PAYLOAD];
}

anna_object_t *anna_complex_create(complex double value)
{
    anna_object_t *obj= anna_object_create_raw(complex_type);
    anna_complex_set(obj, value);
    return obj;
}

inline complex double anna_complex_get(anna_object_t *this)
{
    complex double result;
    size_t off = this->type->mid_identifier[ANNA_MID_COMPLEX_PAYLOAD]->offset;
    memcpy(&result, &this->member[off], sizeof(complex double));
    return result;
}

ANNA_VM_NATIVE(anna_complex_init, 3)
{
    complex double result = anna_as_float(param[1]) + I * anna_as_float(param[2]);
    size_t off = anna_as_obj(param[0])->type->mid_identifier[ANNA_MID_COMPLEX_PAYLOAD]->offset;
    memcpy(&anna_as_obj(param[0])->member[off], &result, sizeof(complex double));
    return param[0];
}

static anna_entry_t anna_complex_sign(double v)
{
    if(v > 0.0)
    {
	return anna_from_int(1);
    }
    else if(v < 0.0)
    {
	return anna_from_int(-1);
    }
    else{
	return anna_from_int(0);
    }    
}

ANNA_VM_NATIVE(anna_complex_cmp_complex, 2)
{
    if(unlikely( anna_entry_null(param[1])))
    {
        return null_entry;
    }
    complex double c1 = anna_as_complex(param[0]);
    complex double c2 = anna_as_complex(param[1]);

    if(cimag(c1) != cimag(c2))
    {
	return anna_complex_sign(cimag(c1) - cimag(c2));    
    }
    return anna_complex_sign(creal(c1) - creal(c2));    
}

ANNA_VM_NATIVE(anna_complex_cmp_float, 2)
{
    if(unlikely( anna_entry_null(param[1])))
    {
        return null_entry;
    }

    complex double c1 = anna_as_complex(param[0]);
    double v2 = anna_as_float(param[1]);
	    
    if(cimag(c1) != 0.0)
    {
	return anna_complex_sign(cimag(c1));
    }
    else
    {
	return anna_complex_sign(creal(c1) - v2);
    }
}

ANNA_VM_NATIVE(anna_complex_cmp_int, 2)
{
    if(unlikely( anna_entry_null(param[1])))
    {
        return null_entry;
    }

    complex double c1 = anna_as_complex(param[0]);
    double v2 = (double)anna_as_int(param[1]);
	    
    if(cimag(c1) != 0.0)
    {
	return anna_complex_sign(cimag(c1));
    }
    else
    {
	return anna_complex_sign(creal(c1) - v2);
    }
}

ANNA_VM_NATIVE(anna_complex_to_string, 1)
{
    complex double val = anna_as_complex(param[0]);
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"%f %ls i%f", creal(val), cimag(val)<0.0 ? L"-":L"+", fabs(cimag(val)));

    wchar_t *buff = sb_content(&sb);
    wchar_t *comma = wcschr(buff, ',');
    if(comma)
    {
	*comma = '.';
	comma = wcschr(comma+1, ',');
	if(comma)
	{
	    *comma = '.';
	}
    }

    return anna_from_obj(anna_string_create(sb_count(&sb), buff));
}

ANNA_VM_NATIVE(anna_complex_hash, 1)
{
    complex double cmp = anna_complex_get(anna_as_obj_fast(param[0]));
    int res = anna_hash((int *)&cmp, sizeof(complex double) / sizeof(int));
    return anna_from_int(res);
}

ANNA_VM_NATIVE(anna_complex_convert_int, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    
    mpz_t *int_val = anna_int_unwrap(anna_as_obj(param[0]));
    double res = mpz_get_d(*int_val);
    return anna_from_obj(anna_complex_create((complex double) res));
}

ANNA_VM_NATIVE(anna_complex_convert_float, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    
    double res  = anna_as_float(param[0]);
    return anna_from_obj(anna_complex_create((complex double) res));
}

ANNA_VM_NATIVE(anna_complex_convert_complex, 1)
{
    return param[0];
}

ANNA_VM_NATIVE(anna_complex_convert_string, 1)
{
    int ok = 1;
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
    char *end2=0;

    char *nstr2 = nstr;

    double i_sign = 1.0;
    int newerr = 0;
    double real = 0.0;
    double imag = 0.0;
    while(*nstr2 && *nstr2 != ' ')
	nstr2++;
    if(*nstr)
    {
	*nstr2 = 0;
	nstr2++;
	if(strncmp(nstr2, "+ i", 3) == 0)
	{
	    nstr2 += 3;
	}
	else if(strncmp(nstr2, "- i", 3) == 0)
	{
	    nstr2 += 3;
	    i_sign = -1.0;
	}
	else
	{
	    goto CLEANUP;
	}	
    }
    
    int olderr = errno;
    errno=0;
    real = strtod(nstr, &end);
    imag = strtod(nstr2, &end2);
    newerr = errno;
    errno=olderr;
    
  CLEANUP:
    if( (newerr) || ( end == 0 ) || ( *end != 0 )|| ( end2 == 0 ) || ( *end2 != 0 ))
    {
	ok = 0;
    }
    
    free(str);
    free(nstr);
    
    return ok ? anna_from_obj(anna_complex_create((complex double)real + I * i_sign*imag)) : null_entry;
}

void anna_complex_type_create()
{
    anna_member_create_blob(
	complex_type, ANNA_MID_COMPLEX_PAYLOAD,
	0, sizeof(complex double));

    anna_type_t *argv[] = 
	{
	    complex_type,
	    float_type,
	    float_type
	}
    ;
    
    wchar_t *argn[]=
	{
	    L"this", L"real", L"imag"
	}
    ;

    anna_type_t *cc_argv[] = 
	{
	    complex_type,
	    complex_type,
	}
    ;
    
    anna_type_t *cf_argv[] = 
	{
	    complex_type,
	    float_type,
	}
    ;
    
    anna_type_t *ci_argv[] = 
	{
	    complex_type,
	    int_type,
	}
    ;
    
    wchar_t *c_argn[]=
	{
	    L"this", L"other"
	}
    ;
    
    anna_type_make_sendable(complex_type);

    anna_type_document(
	complex_type,
	L"The Complex type is the basic complex floating point number type of the Anna language.");
    
    anna_member_create_native_method(
	complex_type,
	anna_mid_get(L"__init__"),
	0,
	&anna_complex_init, 
	complex_type,
	3, argv, argn, 0, 0);
    
    mid_t mmid;
    mmid = anna_member_create_native_method(
	complex_type,
	anna_mid_get(L"cmpComplex"),
	0,
	&anna_complex_cmp_complex, 
	int_type,
	2, cc_argv, c_argn, 0, 0);    
    anna_member_alias(complex_type, mmid, L"__cmp__");
    
    mmid = anna_member_create_native_method(
	complex_type,
	anna_mid_get(L"cmpFloat"),
	0,
	&anna_complex_cmp_float, 
	int_type,
	2, cf_argv, c_argn, 0, 0);    
    anna_member_alias(complex_type, mmid, L"__cmp__");
    
    mmid = anna_member_create_native_method(
	complex_type,
	anna_mid_get(L"cmpInt"),
	0,
	&anna_complex_cmp_int, 
	int_type,
	2, ci_argv, c_argn, 0, 0);    
    anna_member_alias(complex_type, mmid, L"__cmp__");
    
    anna_member_create_native_method(
	complex_type,
	ANNA_MID_TO_STRING,
	0,
	&anna_complex_to_string, 
	string_type, 1, argv, argn, 0, 0);    

    anna_member_create_native_method(
	complex_type,
	ANNA_MID_HASH_CODE,
	0,
	&anna_complex_hash, 
	int_type, 1, argv, argn, 0, 0);

    wchar_t *conv_argn[]=
	{
	    L"value"
	}
    ;

    mmid = anna_member_create_native_type_method(
	complex_type, anna_mid_get(L"convertInt"),
	0, &anna_complex_convert_int,
	complex_type, 1, &int_type, conv_argn, 0,
	L"Convert the specified Integer value to a complex number.");
    anna_member_alias(complex_type, mmid, L"convert");

    mmid = anna_member_create_native_type_method(
	complex_type, anna_mid_get(L"convertString"),
	0, &anna_complex_convert_string,
	complex_type, 1, &string_type, conv_argn, 0,
	L"Convert the specified character String to a complex number. The string needs to be in the form «A + iB» or «A - iB», where A and B represent valid floating point numbers as accepted by Float::convert().");
    anna_member_alias(complex_type, mmid, L"convert");

    mmid = anna_member_create_native_type_method(
	complex_type, anna_mid_get(L"convertFloat"),
	0, &anna_complex_convert_float,
	complex_type, 1, &float_type, conv_argn, 0,
	L"Convert the specified floating point number to a complex number.");
    anna_member_alias(complex_type, mmid, L"convert");

    mmid = anna_member_create_native_type_method(
	complex_type, anna_mid_get(L"convertComplex"),
	0, &anna_complex_convert_complex,
	complex_type, 1, &complex_type, conv_argn, 0,
	L"Convert the specified complex number to a complex number. (This is a no-op.)");
    anna_member_alias(complex_type, mmid, L"convert");
    anna_complex_type_i_create();
}
