


#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna_function.h"
#include "anna_macro.h"

anna_object_t *anna_function_wrap(anna_function_t *result)
{
    return result->wrapper;
}

anna_function_t *anna_function_unwrap(anna_object_t *obj)
{
    assert(obj);    
    anna_function_t **function_ptr = (anna_function_t **)anna_member_addr_get_mid(obj, ANNA_MID_FUNCTION_WRAPPER_PAYLOAD);
    if(function_ptr) 
    {
	//wprintf(L"Got object of type %ls with native method payload\n", obj->type->name);
	return *function_ptr;
    }
    else 
    {
	anna_object_t **function_wrapper_ptr = anna_static_member_addr_get_mid(obj->type, ANNA_MID_CALL_PAYLOAD);
	if(function_wrapper_ptr)
	{
	    //wprintf(L"Got object with __call__ member\n");
	    return anna_function_unwrap(*function_wrapper_ptr);	    
	}
	return 0;	
    }
//     FIXME: Is there any validity checking we could do here?
  
}

anna_function_type_key_t *anna_function_unwrap_type(anna_type_t *type)
{
    if(!type)
    {
	wprintf(L"Critical: Tried to get function from non-existing type\n");
	CRASH;
    }
    
    //wprintf(L"Find function signature for call %ls\n", type->name);
    
    anna_function_type_key_t **function_ptr = (anna_function_type_key_t **)anna_static_member_addr_get_mid(type, ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
    if(function_ptr) 
    {
	//wprintf(L"Got member, has return type %ls\n", (*function_ptr)->result->name);
	return *function_ptr;
    }
    else 
    {
	//wprintf(L"Not a direct function, check for __call__ member\n");
	anna_object_t **function_wrapper_ptr = anna_static_member_addr_get_mid(type, ANNA_MID_CALL_PAYLOAD);
	if(function_wrapper_ptr)
	{
	    //wprintf(L"Found, we're unwrapping it now\n");
	    return anna_function_unwrap_type((*function_wrapper_ptr)->type);	    
	}
	return 0;	
    }
//     FIXME: Is there any validity checking we could do here?
  
}


