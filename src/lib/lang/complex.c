
#include "autogen/anna_complex_i.c"

static inline void anna_complex_set(anna_object_t *this, complex double value)
{
    size_t off = this->type->mid_identifier[ANNA_MID_COMPLEX_PAYLOAD]->offset;
    memcpy(&this->member[off], &value, sizeof(complex double));
}

int anna_is_complex(anna_entry_t *this)
{
    anna_object_t *obj = anna_as_obj(this);
    return !!obj->type->mid_identifier[ANNA_MID_COMPLEX_PAYLOAD];
}

anna_object_t *anna_complex_create(complex double value)
{
    anna_object_t *obj= anna_object_create(complex_type);
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

ANNA_VM_NATIVE(anna_complex_cmp, 2)
{
    if(unlikely( anna_entry_null(param[1])))
    {
        return null_entry;
    }
    else if(anna_is_complex(param[1]))
    {
	complex double v1 = anna_as_complex(param[0]);
	complex double v2 = anna_as_complex(param[1]);
	if(v1 == v2)
	{
	    return anna_from_int(0);
	}
	else{
	    return null_entry;
	}
    }
    else if(anna_is_float(param[1]) || anna_is_int(param[1]))
    {
	complex double c1 = anna_as_complex(param[0]);
	double v2;
	if(anna_is_float(param[1]))
	{
	    v2 = anna_as_float(param[1]);
	}
	else
	{
	    v2 = (double)anna_as_int(param[1]);
	}
	    
	if(cimag(c1) != 0.0)
	{
	    return null_entry;
	}
	else
	{
	    double v1 = creal(c1);
	    
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
    }
    else
    {
	return null_entry;
    }
}

ANNA_VM_NATIVE(anna_complex_to_string, 1)
{
    complex double val = anna_as_complex(param[0]);
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"%f + i%f", creal(val), cimag(val));

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

    return anna_from_obj(anna_string_create(sb_length(&sb), buff));
}

ANNA_VM_NATIVE(anna_complex_hash, 1)
{
    complex double cmp = anna_complex_get(anna_as_obj_fast(param[0]));
    int res = anna_hash((int *)&cmp, sizeof(complex double) / sizeof(int));
    return anna_from_int(res);
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

    anna_type_t *c_argv[] = 
	{
	    complex_type,
	    object_type,
	}
    ;
    
    wchar_t *c_argn[]=
	{
	    L"this", L"other"
	}
    ;
    
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
    
    anna_member_create_native_method(
	complex_type,
	anna_mid_get(L"__cmp__"),
	0,
	&anna_complex_cmp, 
	int_type,
	2, c_argv, c_argn, 0, 0);    
    
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

    anna_complex_type_i_create();
}
