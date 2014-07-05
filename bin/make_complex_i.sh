#! /bin/bash


echo "
/*
  WARNING! This file is automatically generated by the make_anna_complex_i.sh script.
  Do not edit it directly, your changes will be overwritten!
*/
"

init="
    mid_t mmid;
    anna_function_t *fun;

    anna_type_t *argv[]=
	{
	    complex_type,
	    complex_type
	}
    ;
    
    wchar_t *argn[]=
	{
	  L\"this\", L\"param\"
	}
    ;

    anna_type_t *i_argv[]=
	{
	    complex_type,
	    int_type
	}
    ;
    
    wchar_t *i_argn[]=
	{
	  L\"this\", L\"param\"
	}
    ;

    anna_type_t *f_argv[]=
	{
	    complex_type,
	    float_type
	}
    ;
    
    wchar_t *f_argn[]=
	{
	  L\"this\", L\"param\"
	}
    ;

"

init="$init
"

for i in "__add__ v1 + v2" "__increaseAssign__ v1 + v2" "__sub__ v1 - v2" "__decreaseAssign__ v1 - v2" "__mul__ v1 * v2" "__div__ v1 / v2" "exp cpow(v1, v2)"; do
    external_name=$(echo "$i"|cut -f 1 -d ' ')
    name=$(echo $external_name| tr -d _)
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init

    mmid = anna_member_create_native_method(
	complex_type, anna_mid_get(L\"${external_name}Complex__\"), 0,
	&anna_complex_i_${name}, 
	complex_type,
	2, argv, argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(complex_type, mmid)));
    fun->flags |= ANNA_FUNCTION_PURE;
    anna_member_alias(complex_type, mmid, L\"${external_name}\");

    mmid = anna_member_create_native_method(
        complex_type, anna_mid_get(L\"${external_name}Int__\"), 0, 
	&anna_complex_i_int_${name}, 
	complex_type,
	2, i_argv, i_argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(complex_type, mmid)));
    fun->flags |= ANNA_FUNCTION_PURE;
    anna_member_alias(complex_type, mmid, L\"${external_name}\");

    mmid = anna_member_create_native_method(
	complex_type, anna_mid_get(L\"${external_name}Float__\"), 0, 
	&anna_complex_i_float_${name}, 
	complex_type,
	2, f_argv, f_argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(complex_type, mmid)));
    fun->flags |= ANNA_FUNCTION_PURE;
    anna_member_alias(complex_type, mmid, L\"${external_name}\");

    mmid = anna_member_create_native_method(
        complex_type, anna_mid_get(L\"${external_name}IntReverse__\"), 0, 
	&anna_complex_i_int_reverse_${name}, 
	complex_type,
	2, i_argv, i_argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(complex_type, mmid)));
    fun->flags |= ANNA_FUNCTION_PURE;
    anna_member_alias_reverse(complex_type, mmid, L\"${external_name}\");

    mmid = anna_member_create_native_method(
	complex_type, anna_mid_get(L\"${external_name}FloatReverse__\"), 0, 
	&anna_complex_i_float_reverse_${name}, 
	complex_type,
	2, f_argv, f_argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(complex_type, mmid)));
    fun->flags |= ANNA_FUNCTION_PURE;
    anna_member_alias_reverse(complex_type, mmid, L\"${external_name}\");

"

    echo "

ANNA_VM_NATIVE(anna_complex_i_$name, 2)
{
    anna_object_t *res = null_object;
    if(!anna_entry_null(param[1]))
    {  
        complex double v1 = anna_as_complex(param[0]);
        complex double v2 = anna_as_complex(param[1]);
        res = anna_complex_create($op);
    }
    return anna_from_obj(res);
}

ANNA_VM_NATIVE(anna_complex_i_int_$name, 2)
{
    anna_object_t *res = null_object;
    if(!anna_entry_null(param[1]))
    {  
        complex double v1 = anna_as_complex(param[0]);
        complex double v2 = (complex double)anna_as_int(param[1]);
        res = anna_complex_create($op);
    }
    return anna_from_obj(res);
}

ANNA_VM_NATIVE(anna_complex_i_float_$name, 2)
{
    anna_object_t *res = null_object;
    if(!anna_entry_null(param[1]))
    {  
        complex double v1 = anna_as_complex(param[0]);
        complex double v2 = (complex double)anna_as_float(param[1]);
        res = anna_complex_create($op);
    }
    return anna_from_obj(res);
}

ANNA_VM_NATIVE(anna_complex_i_int_reverse_$name, 2)
{
    anna_object_t *res = null_object;
    if(!anna_entry_null(param[1]))
    {  
        complex double v2 = anna_as_complex(param[0]);
        complex double v1 = (complex double)anna_as_int(param[1]);
        res = anna_complex_create($op);
    }
    return anna_from_obj(res);
}

ANNA_VM_NATIVE(anna_complex_i_float_reverse_$name, 2)
{
    anna_object_t *res = null_object;
    if(!anna_entry_null(param[1]))
    {  
        complex double v2 = anna_as_complex(param[0]);
        complex double v1 = (complex double)anna_as_float(param[1]);
        res = anna_complex_create($op);
    }
    return anna_from_obj(res);
}

"
done

init="$init
"

for i in "__neg__ -v"; do
    external_name=$(echo "$i"|cut -f 1 -d ' ')
    name=$(echo $external_name| tr -d _)
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    mmid = anna_member_create_native_method(
	complex_type, anna_mid_get(L\"${external_name}\"), 0, 
	&anna_complex_i_${name}, 
	complex_type,
	1, argv, argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(complex_type, mmid)));
    fun->flags |= ANNA_FUNCTION_PURE;
"

    echo "
ANNA_VM_NATIVE(anna_complex_i_$name, 1)
{
    complex double v = anna_as_complex(param[0]);    
    return anna_from_obj(anna_complex_create($op));
}
"
done

for i in "abs cabs(v)" ; do
    external_name=$(echo "$i"|cut -f 1 -d ' ')
    name=$(echo $external_name| tr -d _)
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    mmid = anna_member_create_native_method(
	complex_type, anna_mid_get(L\"${external_name}\"), 0, 
	&anna_complex_i_${name}, 
	float_type,
	1, argv, argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(complex_type, mmid)));
    fun->flags |= ANNA_FUNCTION_PURE;
"

    echo "
ANNA_VM_NATIVE(anna_complex_i_$name, 1)
{
    if(anna_entry_null(param[1])) return null_entry;
    complex double v = anna_as_complex(param[0]);
    return anna_from_float($op);
}
"
done

echo "
static void anna_complex_type_i_create()
{
$init
}"

