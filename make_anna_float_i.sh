#! /bin/bash


echo "
/*
  WARNING! This file is automatically generated by the make_anna_float_i.sh script.
  Do not edit it directly, your changes will be overwritten!
*/
"

init="
    anna_type_t *argv[]=
	{
	    float_type,
	    float_type
	}
    ;
    
    wchar_t *argn[]=
	{
	  L\"this\", L\"param\"
	}
    ;

    anna_type_t *i_argv[]=
	{
	    float_type,
	    int_type
	}
    ;
    
    wchar_t *i_argn[]=
	{
	  L\"this\", L\"param\"
	}
    ;

"

for i in "gt >" "lt <" "eq ==" "gte >=" "lte <=" "neq !="; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2 -d ' ')
    
    init="$init
    anna_native_method_create(
	float_type, -1, L\"__${name}__Float__\", 0, 
	&anna_float_i_${name}, 
	float_type,
	2, argv, argn);"

    echo "
static anna_object_t *anna_float_i_$name(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    double v1 = anna_float_get(param[0]);
    double v2 = anna_float_get(param[1]);
    return v1 $op v2?param[0]:null_object;
}
"

done

init="$init
"

for i in "add v1 + v2" "increaseAssign v1 + v2" "sub v1 - v2" "decreaseAssign v1 - v2" "mul v1 * v2" "div v1 / v2" "exp pow(v1, v2)"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_native_method_create(
	float_type, -1, L\"__${name}__Float__\", 0,
	&anna_float_i_${name}, 
	float_type,
	2, argv, argn);
    anna_native_method_create(
        float_type, -1, L\"__${name}__Int__\", 0, 
	&anna_float_i_int_${name}, 
	float_type,
	2, i_argv, i_argn);
    anna_native_method_create(
	float_type, -1, L\"__r${name}__Int__\", 0, 
	&anna_float_i_int_r${name}, 
	float_type,
	2, i_argv, i_argn);
"

    echo "
static anna_object_t *anna_float_i_$name(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    double v1 = anna_float_get(param[0]);
    double v2 = anna_float_get(param[1]);
    return anna_float_create($op);
}
static anna_object_t *anna_float_i_int_$name(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    double v1 = anna_float_get(param[0]);
    double v2 = (double)anna_int_get(param[1]);
    return anna_float_create($op);
}
static anna_object_t *anna_float_i_int_r$name(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    double v1 = (double)anna_int_get(param[1]);
    double v2 = anna_float_get(param[0]);
    return anna_float_create($op);
}
"
done

init="$init
"

for i in "abs fabs(v)" "neg -v" "sqrt sqrt(v)" "tan tan(v)" "atan atan(v)" "sin sin(v)" "cos cos(v)" "ln log(v)" "sign (v==0.0?0.0:(v>0?1.0:-1.0))"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_native_method_create(
	float_type, -1, L\"__${name}__\", 0, 
	&anna_float_i_${name}, 
	float_type,
	1, argv, argn);"

    echo "
static anna_object_t *anna_float_i_$name(anna_object_t **param)
{
    double v = anna_float_get(param[0]);
    return anna_float_create($op);
}
"
done

echo "
static void anna_float_type_i_create(anna_stack_frame_t *stack)
{
$init
}"

