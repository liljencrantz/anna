
#include "autogen/object_i.c"

ANNA_VM_NATIVE(anna_object_init, 1)
{
    return param[0];
}

ANNA_VM_NATIVE(anna_object_cmp, 2)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    return anna_from_int(this->type - anna_as_obj(param[1])->type);
}

ANNA_VM_NATIVE(anna_object_hash, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    return anna_from_obj(anna_int_create(hash_ptr_func(this->type)));
}

ANNA_VM_NATIVE(anna_object_to_string, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    string_buffer_t sb;
    sb_init(&sb);
    sb_printf(&sb, L"Object of type %ls", this->type->name);
    anna_entry_t *res = anna_from_obj(anna_string_create(sb_length(&sb), sb_content(&sb)));
    sb_destroy(&sb);
    return res;
}

ANNA_VM_NATIVE(anna_object_type, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    return anna_from_obj(anna_type_wrap(this->type));
}

void anna_object_type_create()
{

    anna_type_t *argv[] = 
	{
	    object_type, object_type
	}
    ;
    
    wchar_t *argn[]=
	{
	    L"this", L"other"
	}
    ;

    anna_type_document(
	object_type,
	L"The Object type is the base type of all other types in Anna.");
    
    anna_type_document(
	object_type,
	L"There is rarely any point in instantiating an object of the Object type.");

    anna_member_create_native_method(
	object_type, anna_mid_get(L"__init__"),
	0, &anna_object_init, object_type, 1,
	argv, argn, 0,
	L"Constructor for the specified type. This method is run during object creation and should be overloaded to perform object setup.");
    
    anna_member_create_native_method(
	object_type, ANNA_MID_HASH_CODE, 0,
	&anna_object_hash, int_type, 1, argv,
	argn, 0,
	L"Hash function. Should always return the same number for the same object and should also return the same number for two equal objects.");

    anna_member_create_native_method(
	object_type,
	ANNA_MID_CMP,
	0,
	&anna_object_cmp,
	int_type,
	2,
	argv,
	argn, 0,
	L"Comparison method. Should return a negative number, zero or a positive number if the compared object is smaller than, equal to or greater than the object being called, respectively. If the objects can't be compared, null should be returned.");
    
    anna_member_create_native_method(
	object_type,
	ANNA_MID_TO_STRING,
	0,
	&anna_object_to_string,
	string_type,
	1,
	argv,
	argn, 0,
	L"String conversion. Called by the String::convert method.");
    
    anna_member_create_native_property(
	object_type, anna_mid_get(L"__type__"),
	type_type, &anna_object_type, 0,
	L"The type of this object");
    
    anna_object_type_i_create();
    anna_type_object_is_created();

}

