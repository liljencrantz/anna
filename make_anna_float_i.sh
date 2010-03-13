#! /bin/bash


echo "
/*
  WARNING! This file is automatically generated by the make_anna_float_i.sh script.
  Do not edit it directly, your changes will be overwritten!
*/
"

init="
    anna_node_t *argv[]=
	{
	    (anna_node_t *)anna_node_identifier_create(0, L\"Float\"), 
	    (anna_node_t *)anna_node_identifier_create(0, L\"Float\"), 
	}
    ;
    
    wchar_t *argn[]=
	{
	  L\"this\", L\"param\"
	}
    ;

    anna_node_t *i_argv[]=
	{
	    (anna_node_t *)anna_node_identifier_create(0, L\"Float\"), 
	    (anna_node_t *)anna_node_identifier_create(0, L\"Int\"), 
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
    anna_native_method_add_node(
	definition, -1, L\"__${name}Float__\", 0, 
	(anna_native_t)&anna_float_i_${name}, 
	(anna_node_t *)anna_node_identifier_create(0, L\"Float\"), 
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

for i in "add v1 + v2" "sub v1 - v2" "mul v1 * v2" "div v1 / v2" "exp pow(v1, v2)"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_native_method_add_node(
	definition, -1, L\"__${name}Float__\", 0,
	(anna_native_t)&anna_float_i_${name}, 
	(anna_node_t *)anna_node_identifier_create(0, L\"Float\"), 
	2, argv, argn);"

    echo "
static anna_object_t *anna_float_i_$name(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    double v1 = anna_float_get(param[0]);
    double v2 = anna_float_get(param[1]);
    return anna_float_create($op);
}
"
done

for i in "add v1 + v2" "sub v1 - v2" "mul v1 * v2" "div v1 / v2" "exp pow(v1, v2)"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_native_method_add_node(
	definition, -1, L\"__${name}Int__\", 0, 
	(anna_native_t)&anna_float_i_int_${name}, 
	(anna_node_t *)anna_node_identifier_create(0, L\"Float\"), 
	2, i_argv, i_argn);"

    echo "
static anna_object_t *anna_float_i_int_$name(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    double v1 = anna_float_get(param[0]);
    double v2 = (double)anna_int_get(param[1]);
    return anna_float_create($op);
}
"
done

for i in "radd v1 + v2" "rsub v1 - v2" "rmul v1 * v2" "rdiv v1 / v2" "rexp pow(v1, v2)"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_native_method_add_node(
	definition, -1, L\"__${name}Int__\", 0, 
	(anna_native_t)&anna_float_i_int_${name}, 
	(anna_node_t *)anna_node_identifier_create(0, L\"Float\"), 
	2, i_argv, i_argn);"

    echo "
static anna_object_t *anna_float_i_int_$name(anna_object_t **param)
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
for i in "abs fabs(v1)" "neg -v1" "sqrt sqrt(v1)" "sign v1==0?0:(v1>0?1.0:-1.0)"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_native_method_add_node(
	definition, -1, L\"__${name}__\", 0, 
	(anna_native_t)&anna_float_i_${name}, 
	(anna_node_t *)anna_node_identifier_create(0, L\"Float\"), 
	1, argv, argn);"

    echo "
static anna_object_t *anna_float_i_$name(anna_object_t **param)
{
    double v1 = anna_float_get(param[0]);
    return anna_int_create($op);
}
"
done



echo "
static void anna_float_type_i_create(anna_node_call_t *definition, anna_stack_frame_t *stack)
{
$init
}"

