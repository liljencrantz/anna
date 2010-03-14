#! /bin/bash


echo "
/*
  WARNING! This file is automatically generated by the make_anna_int_i.sh script.
  Do not edit it directly, your changes will be overwritten!
*/
"

init="

    anna_node_t *argv[]=
	{
	    (anna_node_t *)anna_node_identifier_create(0, L\"Int\"), 
	    (anna_node_t *)anna_node_identifier_create(0, L\"Int\"), 
	}
    ;
    
    wchar_t *argn[]=
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
	definition, -1, L\"__${name}__Int__\", 0, 
	(anna_native_t)&anna_int_i_${name}, 
	(anna_node_t *)anna_node_identifier_create(0, L\"Int\"), 
	2, argv, argn);"

    echo "
static anna_object_t *anna_int_i_$name(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    int v1 = anna_int_get(param[0]);
    int v2 = anna_int_get(param[1]);
    return v1 $op v2?param[0]:null_object;
}
"

done

init="$init
"
for i in "add v1 + v2" "sub v1 - v2" "mul v1 * v2" "div v1 / v2" "shl v1 << v2" "shr v1 >> v2" "mod v1 % v2" "bitand v1 & v2" "bitor v1 | v2" "xor v1 ^ v2" "cshl (v1 << v2) | (v1 >> (32-v2))" "cshr (v1 >> v2) | (v1 << (32-v2))"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_native_method_add_node(
	definition, -1, L\"__${name}__Int__\", 0, 
	(anna_native_t)&anna_int_i_${name}, 
	(anna_node_t *)anna_node_identifier_create(0, L\"Int\"), 
	2, argv, argn);"

    echo "
static anna_object_t *anna_int_i_$name(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    int v1 = anna_int_get(param[0]);
    int v2 = anna_int_get(param[1]);
    return anna_int_create($op);
}
"
done


init="$init
"
for i in "increase v1+v2" "decrease v1-v2"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_native_method_add_node(
	definition, -1, L\"__${name}__Int__\", 0, 
	(anna_native_t)&anna_int_i_${name}, 
	(anna_node_t *)anna_node_identifier_create(0, L\"Int\"), 
	2, argv, argn);"

    echo "
static anna_object_t *anna_int_i_$name(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    int v1 = anna_int_get(param[0]);
    int v2 = anna_int_get(param[1]);
    anna_int_set(param[0], $op);
    return param[0];
}
"
done


init="$init
"
for i in "abs abs(v1)" "neg -v1" "bitnot ~v1" "sign v1==0?0:(v1>0?1:-1)" ; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_native_method_add_node(
	definition, -1, L\"__${name}__\", 0, 
	(anna_native_t)&anna_int_i_${name}, 
	(anna_node_t *)anna_node_identifier_create(0, L\"Int\"), 
	1, argv, argn);"

    echo "
static anna_object_t *anna_int_i_$name(anna_object_t **param)
{
    int v1 = anna_int_get(param[0]);
    return anna_int_create($op);
}
"
done

init="$init
"
for i in "next v1+1" "prev v1-1" ; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_native_method_add_node(
	definition, -1, L\"__${name}__\", 0, 
	(anna_native_t)&anna_int_i_${name}, 
	(anna_node_t *)anna_node_identifier_create(0, L\"Int\"), 
	1, argv, argn);"

    echo "
static anna_object_t *anna_int_i_$name(anna_object_t **param)
{
    int v1 = anna_int_get(param[0]);
    anna_int_set(param[0], $op);
    return param[0];
}
"
done



echo "
static void anna_int_type_i_create(anna_node_call_t *definition, anna_stack_frame_t *stack)
{
$init
}"

