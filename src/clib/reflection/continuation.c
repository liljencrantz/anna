/*
ANNA_VM_NATIVE(anna_type_to_string, 1)
{
    anna_object_t *this = anna_as_obj_fast(param[0]);
    anna_type_t *type = anna_type_unwrap(this);
    return anna_from_obj(anna_string_create(wcslen(type->name), type->name));
}
*/
static void anna_vmstack_load()
{
//    anna_member_create_blob(vmstack_type, ANNA_MID_VMSTACK_WRAPPER_PAYLOAD, 0, sizeof(void *));
/*
    anna_type_t *argv[] = 
	{
	    type_type, type_type
	}
    ;
    wchar_t *argn[]=
	{
	    L"this", L"other"
	}
    ;
*/
/*    
    anna_member_create_native_property(
	type_type, anna_mid_get(L"name"),
	string_type, &anna_type_to_string, 0, L"The name of this type.");
    anna_member_create_native_property(
	type_type, anna_mid_get(L"isModule"),
	int_type, &anna_type_is_module, 0, L"Is true if this type represents a module.");
    anna_member_create_native_property(
	type_type,
	anna_mid_get(L"member"),
	anna_list_type_get_imutable(member_type),
	&anna_type_i_get_member,
	0, L"A list of all members of this type.");
    
    anna_member_create_native_method(
	type_type, ANNA_MID_HASH_CODE, 0,
	&anna_type_hash, int_type, 1, argv, argn);
    
    anna_member_create_native_method(
	type_type,
	ANNA_MID_CMP,
	0,
	&anna_type_cmp,
	int_type,
	2,
	argv,
	argn);
    anna_member_create_native_method(
	type_type,
	ANNA_MID_TO_STRING,
	0,
	&anna_type_to_string,
	string_type,
	1,
	argv,
	argn);    
    
    anna_member_create_native_method(
	type_type, anna_mid_get(L"abides"), 0,
	&anna_type_abides, object_type, 2, argv,
	argn);   

    anna_member_create_native_property(
	type_type, anna_mid_get(L"attribute"),
	node_call_type,
	&anna_type_i_get_attribute,
	0,
	L"All attributes specified for this type.");
*/

}
