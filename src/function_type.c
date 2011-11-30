
void anna_function_type_print(anna_function_type_t *k)
{
    wprintf(L"def %ls (", k->return_type?k->return_type->name:L"?");
    int i;
    for(i=0;i<k->input_count; i++)
    {
	if(i!=0)
	    wprintf(L", ");
	wprintf(
	    L"%ls %ls",
	    (k->input_type && k->input_type[i])?k->input_type[i]->name:L"?",
	    (k->input_name && k->input_name[i])?k->input_name[i]:L"");
    }
    wprintf(L");\n");
}

__pure anna_function_type_t *anna_function_type_unwrap(anna_type_t *type)
{
    if(!type)
    {
	wprintf(L"Critical: Tried to get function from non-existing type\n");
	CRASH;
    }
    
    anna_function_type_t **function_ptr = 
	(anna_function_type_t **)anna_entry_get_addr_static(
	    type,
	    ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
    if(function_ptr) 
    {
	return *function_ptr;
    }
    return 0;	
}

