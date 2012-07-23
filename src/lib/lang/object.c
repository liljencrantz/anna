
ANNA_VM_NATIVE(anna_object_init, 1)
{
    return param[0];
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
	L"The Object type does not have any members at all. There is rarely any point in instantiating an object of the Object type.");

    anna_member_create_native_method(
	object_type, anna_mid_get(L"__init__"),
	0, &anna_object_init, object_type, 1,
	argv, argn, 0,
	L"Constructor for the specified type. This method is run during object creation and should be overloaded to perform object setup.");

    anna_type_object_is_created();

}
