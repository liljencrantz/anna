#! /bin/bash

echo "
/*
  WARNING! This file is automatically generated by the make_anna_char_i.sh script.
  Do not edit it directly, your changes will be overwritten!
*/
"

init="
    anna_type_t *i_argv[]=
	{
	    char_type,
            int_type
	}
    ;
    
    wchar_t *argn[]=
	{
	  L\"this\", L\"param\"
	}
    ;

    mid_t mmid;
    anna_function_t *fun;
"

init="$init
"

for i in "add +" "sub -" "increaseAssign +" "decreaseAssign -"; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2 -d ' ')
    
    init="$init
    mmid = anna_member_create_native_method(char_type, anna_mid_get(L\"__${name}__Int__\"), 0, &anna_char_i_${name}, char_type, 2, i_argv, argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(char_type, mmid)));
    fun->flags |= ANNA_FUNCTION_PURE;
    anna_member_alias(char_type, mmid, L\"__${name}__\");

"

    echo "
ANNA_VM_NATIVE(anna_char_i_$name, 2)
{
    if(anna_is_obj(param[1]) && anna_as_obj(param[1])==null_object)
        return null_entry;
  
    wchar_t v1 = anna_as_char(param[0]);
    int v2 = anna_as_int(param[1]);
    return anna_from_char(v1 $op v2);
}
"
done

init="$init
"
for i in "nextAssign v+1" "prevAssign v-1" ; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    op=$(echo "$i"|cut -f 2- -d ' ')
    
    init="$init
    anna_member_create_native_method(
	char_type, anna_mid_get(L\"__${name}__\"), 0, 
	&anna_char_i_${name}, 
	char_type,
	1, i_argv, argn, 0, 0);
"

    echo "
ANNA_VM_NATIVE(anna_char_i_$name, 2)
{
    wchar_t v = anna_as_char(param[0]);
    return anna_from_char($op);
}
"
done


for i in "alpha True for alphabetical characters." "alnum True for alphanumerical characters." "blank True for blank characters" "digit True for digits" "cntrl True for control characters" "lower True for lower case characters." "upper True for upper case characters." "graph True for any printable character except space" "print True for any printable character including space" "punct True for any printable character which is not alphanumeric." "space True for white space characters." "xdigit True for hexadecimal digits."; do
    name=$(echo "$i"|cut -f 1 -d ' ')
    desc=$(echo "$i"|cut -f 2- -d ' ')

    init="$init
    anna_member_create_native_property(
	char_type,
	anna_mid_get(L\"${name}?\"),
	int_type,
	&anna_char_i_q_$name,
	0,
	L\"${desc}\");
"
    echo "
ANNA_VM_NATIVE(anna_char_i_q_$name, 1)
{
    wchar_t v = anna_as_char(param[0]);
    return iswctype(v, wctype(\"$name\")) ? anna_from_int(1): null_entry;
}
"
done


echo "
static void anna_char_type_i_create()
{
$init
}"

