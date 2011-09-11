
/*
  WARNING! This file is automatically generated by the make_anna_char_i.sh script.
  Do not edit it directly, your changes will be overwritten!
*/


ANNA_VM_NATIVE(anna_char_i_add, 2)
{
    if(anna_is_obj(param[1]) && anna_as_obj(param[1])==null_object)
        return null_entry;
  
    wchar_t v1 = anna_as_char(param[0]);
    int v2 = anna_as_int(param[1]);
    return anna_from_char(v1 + v2);
}


ANNA_VM_NATIVE(anna_char_i_sub, 2)
{
    if(anna_is_obj(param[1]) && anna_as_obj(param[1])==null_object)
        return null_entry;
  
    wchar_t v1 = anna_as_char(param[0]);
    int v2 = anna_as_int(param[1]);
    return anna_from_char(v1 - v2);
}


ANNA_VM_NATIVE(anna_char_i_increaseAssign, 2)
{
    if(anna_is_obj(param[1]) && anna_as_obj(param[1])==null_object)
        return null_entry;
  
    wchar_t v1 = anna_as_char(param[0]);
    int v2 = anna_as_int(param[1]);
    return anna_from_char(v1 + v2);
}


ANNA_VM_NATIVE(anna_char_i_decreaseAssign, 2)
{
    if(anna_is_obj(param[1]) && anna_as_obj(param[1])==null_object)
        return null_entry;
  
    wchar_t v1 = anna_as_char(param[0]);
    int v2 = anna_as_int(param[1]);
    return anna_from_char(v1 - v2);
}


ANNA_VM_NATIVE(anna_char_i_nextAssign, 2)
{
    wchar_t v = anna_as_char(param[0]);
    return anna_from_char(v+1);
}


ANNA_VM_NATIVE(anna_char_i_prevAssign, 2)
{
    wchar_t v = anna_as_char(param[0]);
    return anna_from_char(v-1);
}


static void anna_char_type_i_create()
{

    anna_type_t *i_argv[]=
	{
	    char_type,
            int_type
	}
    ;
    
    wchar_t *argn[]=
	{
	  L"this", L"param"
	}
    ;

    mid_t mmid;
    anna_function_t *fun;


    mmid = anna_member_create_native_method(char_type, anna_mid_get(L"__add__Int__"), 0, &anna_char_i_add, char_type, 2, i_argv, argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(char_type, mmid)));
    anna_function_alias_add(fun, L"__add__");


    mmid = anna_member_create_native_method(char_type, anna_mid_get(L"__sub__Int__"), 0, &anna_char_i_sub, char_type, 2, i_argv, argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(char_type, mmid)));
    anna_function_alias_add(fun, L"__sub__");


    mmid = anna_member_create_native_method(char_type, anna_mid_get(L"__increaseAssign__Int__"), 0, &anna_char_i_increaseAssign, char_type, 2, i_argv, argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(char_type, mmid)));
    anna_function_alias_add(fun, L"__increaseAssign__");


    mmid = anna_member_create_native_method(char_type, anna_mid_get(L"__decreaseAssign__Int__"), 0, &anna_char_i_decreaseAssign, char_type, 2, i_argv, argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(char_type, mmid)));
    anna_function_alias_add(fun, L"__decreaseAssign__");



    anna_member_create_native_method(
	char_type, anna_mid_get(L"__nextAssign__"), 0, 
	&anna_char_i_nextAssign, 
	char_type,
	1, i_argv, argn, 0, 0);

    anna_member_create_native_method(
	char_type, anna_mid_get(L"__prevAssign__"), 0, 
	&anna_char_i_prevAssign, 
	char_type,
	1, i_argv, argn, 0, 0);

}
