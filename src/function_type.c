
void anna_function_type_print(anna_function_type_t *k)
{
    string_buffer_t sb;
    sb_init(&sb);
    ANNA_FUNCTION_PROTOTYPE(k, &sb);
    anna_message(L"%ls\n", sb_content(&sb));
    sb_destroy(&sb);
}

__pure anna_function_type_t *anna_function_type_unwrap(anna_type_t *type)
{
    if(!type)
    {
	anna_message(L"Critical: Tried to get function from non-existing type\n");
	CRASH;
    }
    if(type == null_type)
    {
	anna_type_t *argv[] = 
	    {
		anna_hash_type_get(string_type, any_type),
		any_type
	    }
	;
	wchar_t *argn[] = 
	    {
		L"named", L"unnamed"
	    }
	;
	
	type = anna_type_get_function(
	    null_type,
	    2,
	    argv,
	    argn,
	    0,
	    ANNA_FUNCTION_VARIADIC | ANNA_FUNCTION_VARIADIC_NAMED);
	return anna_function_type_unwrap(type);
    }
    
    anna_entry_t *function_ptr = 
	anna_entry_get_addr_static(
	    type,
	    ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);

    if(function_ptr && !anna_entry_null(*function_ptr))
    {
	return (anna_function_t *)anna_as_ptr(*function_ptr);
    }
    return 0;	
}

__pure anna_function_type_t *anna_function_type_of_function(anna_function_t *fun)
{
    return anna_function_type_unwrap(anna_function_wrap(fun)->type);
}
