#! /bin/bash


echo "
/*
  WARNING! This file is automatically generated by the make_anna_complex_i.sh script.
  Do not edit it directly, your changes will be overwritten!
*/
"

init="
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

for i in "eq ==" "neq !="; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2 -d ' ')
    
    init="$init
    anna_native_method_create(
	complex_type, -1, L\"__${name}__Complex__\", 0, 
	&anna_complex_i_${name}, 
	complex_type,
	2, argv, argn);"

    echo "
static anna_object_t *anna_complex_i_$name(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = anna_complex_get(param[1]);
    return v1 $op v2?param[0]:null_object;
}
"

done

init="$init
"

for i in "add v1 + v2" "increaseAssign v1 + v2" "sub v1 - v2" "decreaseAssign v1 - v2" "mul v1 * v2" "div v1 / v2" "exp cpow(v1, v2)"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_native_method_create(
	complex_type, -1, L\"__${name}__Complex__\", 0,
	&anna_complex_i_${name}, 
	complex_type,
	2, argv, argn);
    anna_native_method_create(
        complex_type, -1, L\"__${name}__Int__\", 0, 
	&anna_complex_i_int_${name}, 
	complex_type,
	2, i_argv, i_argn);
    anna_native_method_create(
	complex_type, -1, L\"__${name}__Float__\", 0, 
	&anna_complex_i_float${name}, 
	complex_type,
	2, f_argv, f_argn);
"

    echo "
static anna_object_t *anna_complex_i_$name(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;

    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = anna_complex_get(param[1]);
    return anna_complex_create($op);
}
static anna_object_t *anna_complex_i_int_$name(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = (complex double)anna_int_get(param[1]);
    return anna_complex_create($op);
}
static anna_object_t *anna_complex_i_float$name(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    complex double v1 = anna_complex_get(param[0]);
    complex double v2 = (complex double)anna_float_get(param[1]);
    return anna_complex_create($op);
}
"
done

init="$init
"

for i in "neg -v" "sqrt csqrt(v)" "tan ctan(v)" "atan catan(v)" "sin csin(v)" "cos ccos(v)" "ln clog(v)" ; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_native_method_create(
	complex_type, -1, L\"__${name}__\", 0, 
	&anna_complex_i_${name}, 
	complex_type,
	1, argv, argn);"

    echo "
static anna_object_t *anna_complex_i_$name(anna_object_t **param)
{
    complex double v = anna_complex_get(param[0]);
    return anna_complex_create($op);
}
"
done

for i in "abs cabs(v)" ; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_native_method_create(
	complex_type, -1, L\"__${name}__\", 0, 
	&anna_complex_i_${name}, 
	float_type,
	1, argv, argn);"

    echo "
static anna_object_t *anna_complex_i_$name(anna_object_t **param)
{
    complex double v = anna_complex_get(param[0]);
    return anna_float_create($op);
}
"
done

echo "
static void anna_complex_type_i_create(anna_stack_template_t *stack)
{
$init
}"

