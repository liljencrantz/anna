
void anna_function_type_print(anna_function_type_t *k)
{
    string_buffer_t sb;
    sb_init(&sb);
    ANNA_FUNCTION_PROTOTYPE(k, &sb);
    wprintf(L"%ls\n", sb_content(&sb));
    sb_destroy(&sb);
}

__pure anna_function_type_t *anna_function_type_unwrap(anna_type_t *type)
{
    if(!type)
    {
	wprintf(L"Critical: Tried to get function from non-existing type\n");
	CRASH;
    }
    if(type == null_type)
    {
	return 0;
    }
    
    anna_function_type_t **function_ptr = 
	(anna_function_type_t **)anna_entry_get_addr_static(
	    type,
	    ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);

    if(function_ptr && *function_ptr != (anna_function_type_t *)null_entry) 
    {
	return *function_ptr;
    }
    return 0;	
}

