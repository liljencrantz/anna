#! /bin/bash


echo "
/*
  WARNING! This file is automatically generated by the make_anna_char_i.sh script.
  Do not edit it directly, your changes will be overwritten!
*/
"

init="
    anna_node_t *c_argv[]=
	{
	    (anna_node_t *)anna_node_create_identifier(0, L\"Char\"), 
	    (anna_node_t *)anna_node_create_identifier(0, L\"Char\"), 
	}
    ;
    
    anna_node_t *i_argv[]=
	{
	    (anna_node_t *)anna_node_create_identifier(0, L\"Char\"), 
	    (anna_node_t *)anna_node_create_identifier(0, L\"Int\"), 
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
	definition, -1, L\"__${name}__Char__\", 0, (anna_native_t)&anna_char_i_${name}, 
	(anna_node_t *)anna_node_create_identifier(0, L\"Char\"), 2, c_argv, argn);"

    echo "
static anna_object_t *anna_char_i_$name(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    wchar_t v1 = anna_char_get(param[0]);
    wchar_t v2 = anna_char_get(param[1]);
    return v1 $op v2?param[0]:null_object;
}
"

done

init="$init
"

for i in "add +" "sub -"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2 -d ' ')
    
    init="$init
    anna_native_method_add_node(definition, -1, L\"__${name}__Int__\", 0, (anna_native_t)&anna_char_i_${name}, (anna_node_t *)anna_node_create_identifier(0, L\"Char\"), 2, i_argv, argn);"

    echo "
static anna_object_t *anna_char_i_$name(anna_object_t **param)
{
    if(param[1]==null_object)
        return null_object;
  
    wchar_t v1 = anna_char_get(param[0]);
    int v2 = anna_int_get(param[1]);
    return anna_char_create(v1 $op v2);
}
"
done

echo "
static void anna_char_type_i_create(anna_node_call_t *definition, anna_stack_frame_t *stack)
{
$init
}"

