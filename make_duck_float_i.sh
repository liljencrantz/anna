#! /bin/bash


echo "
/*
  WARNING! This file is automatically generated by the make_duck_float_i.sh script.
  Do not edit it directly, your changes will be overwritten!
*/
"

init="
    duck_type_t *argv[]=
	{
	    float_type, float_type
	}
    ;
    
    wchar_t *argn[]=
	{
	  L\"this\", L\"param\"
	}
    ;

    duck_type_t *i_argv[]=
	{
	    float_type, int_type
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
    duck_native_method_create(float_type, -1, L\"__${name}Float__\", 0, (duck_native_t)&duck_float_i_${name}, float_type, 2, argv, argn);"

    echo "
static duck_object_t *duck_float_i_$name(duck_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    double v1 = duck_float_get(param[0]);
    double v2 = duck_float_get(param[1]);
    return v1 $op v2?param[0]:null_object;
}
"

done

init="$init
"

for i in "add +" "sub -" "mul *" "div /"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2 -d ' ')
    
    init="$init
    duck_native_method_create(float_type, -1, L\"__${name}Float__\", 0, (duck_native_t)&duck_float_i_${name}, float_type, 2, argv, argn);"

    echo "
static duck_object_t *duck_float_i_$name(duck_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    duck_object_t *result = duck_float_create();
    double v1 = duck_float_get(param[0]);
    double v2 = duck_float_get(param[1]);
    duck_float_set(result, v1 $op v2);
    return result;
}
"
done

for i in "add +" "sub -" "mul *" "div /"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2 -d ' ')
    
    init="$init
    duck_native_method_create(float_type, -1, L\"__${name}Int__\", 0, (duck_native_t)&duck_float_i_int_${name}, float_type, 2, i_argv, i_argn);"

    echo "
static duck_object_t *duck_float_i_int_$name(duck_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    duck_object_t *result = duck_float_create();
    double v1 = duck_float_get(param[0]);
    int v2 = duck_int_get(param[1]);
    duck_float_set(result, v1 $op (double)v2);
    return result;
}
"
done

for i in "radd +" "rsub -" "rmul *" "rdiv /"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2 -d ' ')
    
    init="$init
    duck_native_method_create(float_type, -1, L\"__${name}Int__\", 0, (duck_native_t)&duck_float_i_int_${name}, float_type, 2, i_argv, i_argn);"

    echo "
static duck_object_t *duck_float_i_int_$name(duck_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    duck_object_t *result = duck_float_create();
    int v1 = duck_int_get(param[1]);
    double v2 = duck_float_get(param[0]);
    duck_float_set(result, ((double)v1) $op v2);
    return result;
}
"
done

echo "
static void duck_float_type_i_create(duck_stack_frame_t *stack)
{
$init
}"

