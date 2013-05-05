
ANNA_VM_NATIVE(anna_any_init, 1)
{
    return param[0];
}

void anna_any_type_create()
{

    anna_type_t *argv[] = 
	{
	    any_type, any_type
	}
    ;
    
    wchar_t *argn[]=
	{
	    L"this", L"other"
	}
    ;

    anna_type_document(
	any_type,
	L"The Any type is the most basic type of all, it does not own any members at all. There is rarely any point in instantiating an object of the Any type.");

    anna_member_create_native_method(
	any_type, anna_mid_get(L"__init__"),
	0, &anna_any_init, any_type, 1,
	argv, argn, 0,
	L"Constructor for the specified type. This method is run during object creation and should be overloaded to perform object setup.");

    anna_type_object_is_created();

}
