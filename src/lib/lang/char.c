#include "autogen/char_i.c"

anna_object_t *anna_char_create(wchar_t value)
{
    anna_object_t *obj= anna_object_create(char_type);
    anna_char_set(obj, value);
    return obj;
}

void anna_char_set(anna_object_t *this, wchar_t value)
{
    memcpy(anna_entry_get_addr(this,ANNA_MID_CHAR_PAYLOAD), &value, sizeof(wchar_t));
}


wchar_t anna_char_get(anna_object_t *this)
{
    wchar_t result;
    memcpy(&result, anna_entry_get_addr(this,ANNA_MID_CHAR_PAYLOAD), sizeof(wchar_t));
    return result;
}

ANNA_VM_NATIVE(anna_char_i_get_ordinal, 1)
{
    return anna_from_int((int)anna_as_char(param[0]));
}

ANNA_VM_NATIVE(anna_char_i_get_upper, 1)
{
    return anna_from_char(towupper(anna_as_char(param[0])));
}

ANNA_VM_NATIVE(anna_char_i_get_lower, 1)
{
    return anna_from_char(towlower(anna_as_char(param[0])));
}

ANNA_VM_NATIVE(anna_char_cmp, 2)
{
    if(anna_is_char(param[1]) || anna_as_obj(param[1])->type == char_type)
    {
	return anna_from_int(anna_as_char(param[0]) - anna_as_char(param[1]));
    }
    return null_entry;
}

ANNA_VM_NATIVE(anna_char_to_string, 1)
{
    wchar_t ch = anna_as_char(param[0]);
    return anna_from_obj(anna_string_create(1, &ch));
}

ANNA_VM_NATIVE(anna_char_convert, 1)
{
    int ch = anna_as_int(param[0]);
    return anna_from_obj(anna_char_create(ch));
}

ANNA_VM_NATIVE(anna_char_sub_char, 2)
{
    if(anna_is_obj(param[1]) && anna_as_obj(param[1])==null_object)
        return null_entry;
  
    wchar_t v1 = anna_as_char(param[0]);
    wchar_t v2 = anna_as_char(param[1]);
    return anna_from_int(v1 - v2);
}

void anna_char_type_create()
{
    anna_type_t *argv[] = 
	{
	    int_type, object_type
	}
    ;
    wchar_t *argn[]=
	{
	    L"this", L"other"
	}
    ;

    anna_type_document(
	char_type,
	L"The Char type is the basic character type of the Anna language.");
    
    anna_type_document(
	char_type,
	L"Anna Char objects are implemented using wide characters, e.g. the wchar_t type in C. This means that every character takes the same amount of memory. Nearly all Char values, (those with 30 or fewer bits used), are stored directly on the stack and use no heap memory at all.");

    anna_type_document(
	char_type,
	L"Anna Char objects are imutable, meaning their value never changes.");

    anna_member_create(char_type, ANNA_MID_CHAR_PAYLOAD, 0, null_type);

    anna_member_create_native_property(
	char_type, anna_mid_get(L"ordinal"),
	int_type, &anna_char_i_get_ordinal, 0,
	L"The ordinal number of this Char. This property returns the same value as the Int::convert static method.");
    
    anna_member_create_native_property(
	char_type,
	anna_mid_get(L"upper"),
	char_type,
	&anna_char_i_get_upper,
	0,
	L"The upper case equivalent of this Char.");

    anna_member_create_native_property(
	char_type,
	anna_mid_get(L"lower"),
	char_type,
	&anna_char_i_get_lower,
	0,
	L"The lower case eqivalent of this Char.");


    mid_t mmid;
    anna_function_t *fun;

    wchar_t *conv_argn[]=
	{
	    L"value"
	}
    ;

    mmid = anna_member_create_native_type_method(
	char_type, anna_mid_get(L"convert"), 0,
	&anna_char_convert, char_type, 1, &int_type,
	conv_argn, 0,
	L"Creates a new Char object with the specified ordinal number.");



    anna_member_create_native_method(
	char_type, anna_mid_get(L"__cmp__"), 0,
	&anna_char_cmp, int_type, 2, argv, argn, 0, 0); 
    
    anna_member_create_native_method(
	char_type, ANNA_MID_TO_STRING, 0,
	&anna_char_to_string, string_type, 1,
	argv, argn, 0, 0);


    anna_type_t *sub_argv[]=
	{
	    char_type,
            char_type
	}
    ;
    
    wchar_t *sub_argn[]=
	{
	  L"this", L"param"
	}
    ;

    mmid = anna_member_create_native_method(
	char_type, anna_mid_get(L"__sub__Char__"), 0, &anna_char_sub_char, int_type, 2, sub_argv, sub_argn, 0, 0);
    fun = anna_function_unwrap(anna_as_obj_fast(anna_entry_get_static(char_type, mmid)));
    anna_function_alias_add(fun, L"__sub__");

    anna_char_type_i_create();
}
