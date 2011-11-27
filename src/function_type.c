
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
    
//    wprintf(L"Find function signature for call %ls\n", type->name);
    
    anna_function_type_t **function_ptr = 
	(anna_function_type_t **)anna_entry_get_addr_static(
	    type,
	    ANNA_MID_FUNCTION_WRAPPER_TYPE_PAYLOAD);
    if(function_ptr) 
    {
//	wprintf(L"Got member, has return type %ls\n", (*function_ptr)->return_type->name);
	return *function_ptr;
    }
    return 0;	
    FIXME("Missing validity checking")
}

anna_type_t *anna_function_type_each_create(
    wchar_t *name, 
    anna_type_t *key_type,
    anna_type_t *value_type)
{
    anna_function_type_t *each_key = malloc(sizeof(anna_function_type_t) + 2*sizeof(anna_type_t *));
    each_key->return_type = object_type;
    each_key->input_count = 2;
    each_key->flags = 0;

    each_key->input_name = malloc(sizeof(wchar_t *)*2);
    each_key->input_name[0] = L"key";
    each_key->input_name[1] = L"value";

    each_key->input_type[0] = key_type;
    each_key->input_type[1] = value_type;    
    anna_type_t *fun_type = anna_type_native_create(name, stack_global);
    anna_reflection_type_for_function_create(each_key, fun_type);
    return fun_type;
}
