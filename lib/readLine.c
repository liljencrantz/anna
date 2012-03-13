
/*
    DO NOT MANUALLY EDIT THIS FILE.

    This file has been automaticaly generated by the anna bind
    utility. If you manually edit it, your changes will eventually be
    lost. Not to mention the fact that staring at machine generated
    code rots your brain. If this file is incorrect, either update the
    bind utility or update the binding source, which is located in the
    file:

    internalBindings/readLine.bind

 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "anna/anna.h"

// Declare internal variables for all types defined in this module


// Data used to initialize all types defined in this module
const static anna_type_data_t anna_readLine_type_data[] = 
{
};

// This is the source code of the various wrapper functions

static char *anna_readline_wrapper(char *prompt)
{
    static char *prev = 0;
    if(prev) 
        free(prev);
    return prev=readline(prompt);
}

ANNA_VM_NATIVE(readLine_i__read_line, 1)
{
    // Validate parameters
        if(param[0] == null_entry){return null_entry;}


    // Mangle input parameters
    char *native_param_prompt = (param[0] == null_entry) ? 0 : anna_string_payload_narrow(anna_as_obj(param[0]));

    // Validate parameters
    

    // Call the function
    char * tmp_var_1 = anna_readline_wrapper(native_param_prompt);
    anna_entry_t *result = (tmp_var_1) ? anna_from_obj(anna_string_create_narrow(strlen(tmp_var_1), tmp_var_1)) : null_entry;
    // Perform cleanup
    free(native_param_prompt);

    // Return result
    return result;
}
const static anna_type_data_t anna_history_type_data[] = 
{
};

ANNA_VM_NATIVE(readLine_i_history_add, 1)
{
    // Validate parameters
        if(param[0] == null_entry){return null_entry;}


    // Mangle input parameters
    char *native_param_line = (param[0] == null_entry) ? 0 : anna_string_payload_narrow(anna_as_obj(param[0]));

    // Validate parameters
    

    // Call the function
    add_history(native_param_line);
    anna_entry_t *result = null_entry;

    // Perform cleanup
    free(native_param_line);

    // Return result
    return result;
}

ANNA_VM_NATIVE(readLine_i_history_read, 1)
{
    // Validate parameters

    // Mangle input parameters
    char *native_param_filename = (param[0] == null_entry) ? 0 : anna_string_payload_narrow(anna_as_obj(param[0]));

    // Validate parameters
    

    // Call the function
    int tmp_var_2 = read_history(native_param_filename);
    anna_entry_t *result = (tmp_var_2)?anna_from_int(1):null_entry;
    // Perform cleanup
    free(native_param_filename);

    // Return result
    return result;
}

ANNA_VM_NATIVE(readLine_i_history_read_range, 3)
{
    // Validate parameters
        if(param[1] == null_entry){return null_entry;}

        if(param[2] == null_entry){return null_entry;}


    // Mangle input parameters
    char *native_param_filename = (param[0] == null_entry) ? 0 : anna_string_payload_narrow(anna_as_obj(param[0]));
    int native_param_from = anna_as_int(param[1]);
    int native_param_to = anna_as_int(param[2]);

    // Validate parameters
    
    
    

    // Call the function
    int tmp_var_3 = read_history_range(native_param_filename, native_param_from, native_param_to);
    anna_entry_t *result = (tmp_var_3)?anna_from_int(1):null_entry;
    // Perform cleanup
    free(native_param_filename);

    // Return result
    return result;
}

ANNA_VM_NATIVE(readLine_i_history_write, 1)
{
    // Validate parameters

    // Mangle input parameters
    char *native_param_filename = (param[0] == null_entry) ? 0 : anna_string_payload_narrow(anna_as_obj(param[0]));

    // Validate parameters
    

    // Call the function
    int tmp_var_4 = write_history(native_param_filename);
    anna_entry_t *result = (tmp_var_4)?anna_from_int(1):null_entry;
    // Perform cleanup
    free(native_param_filename);

    // Return result
    return result;
}

ANNA_VM_NATIVE(readLine_i_history_append, 2)
{
    // Validate parameters
        if(param[0] == null_entry){return null_entry;}


    // Mangle input parameters
    int native_param_count = anna_as_int(param[0]);
    char *native_param_filename = (param[1] == null_entry) ? 0 : anna_string_payload_narrow(anna_as_obj(param[1]));

    // Validate parameters
    
    

    // Call the function
    int tmp_var_5 = append_history(native_param_count, native_param_filename);
    anna_entry_t *result = (tmp_var_5)?anna_from_int(1):null_entry;
    // Perform cleanup
    free(native_param_filename);

    // Return result
    return result;
}

ANNA_VM_NATIVE(readLine_i_history_truncate_file, 2)
{
    // Validate parameters
        if(param[0] == null_entry){return null_entry;}

        if(param[1] == null_entry){return null_entry;}


    // Mangle input parameters
    char *native_param_filename = (param[0] == null_entry) ? 0 : anna_string_payload_narrow(anna_as_obj(param[0]));
    int native_param_count = anna_as_int(param[1]);

    // Validate parameters
    
    

    // Call the function
    int tmp_var_6 = history_truncate_file(native_param_filename, native_param_count);
    anna_entry_t *result = (tmp_var_6)?anna_from_int(1):null_entry;
    // Perform cleanup
    free(native_param_filename);

    // Return result
    return result;
}

void anna_history_create(anna_stack_template_t *stack);
void anna_history_create(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_history_type_data, stack);        
}
void anna_history_load(anna_stack_template_t *stack);
void anna_history_load(anna_stack_template_t *stack)
{
    mid_t latest_mid;    
    anna_function_t *latest_function;
    anna_type_t *stack_type = anna_stack_wrap(stack)->type;
    anna_module_data_t modules[] =
        {
        };
    anna_module_data_create(modules, stack);

    wchar_t *this_argn[] = {L"this"};


    anna_type_t *readLine_i_history_add_argv[] = {string_type};
    wchar_t *readLine_i_history_add_argn[] = {L"line"};
    latest_function = anna_module_function(stack, L"add", 0, &readLine_i_history_add, object_type, 1, readLine_i_history_add_argv, readLine_i_history_add_argn, 0, 0);

    anna_type_t *readLine_i_history_read_argv[] = {string_type};
    wchar_t *readLine_i_history_read_argn[] = {L"filename"};
    latest_function = anna_module_function(stack, L"read", 0, &readLine_i_history_read, object_type, 1, readLine_i_history_read_argv, readLine_i_history_read_argn, 0, 0);

    anna_type_t *readLine_i_history_read_range_argv[] = {string_type, int_type, int_type};
    wchar_t *readLine_i_history_read_range_argn[] = {L"filename", L"from", L"to"};
    latest_function = anna_module_function(stack, L"readRange", 0, &readLine_i_history_read_range, object_type, 3, readLine_i_history_read_range_argv, readLine_i_history_read_range_argn, 0, 0);
    anna_member_alias(stack_type, anna_mid_get(L"readRange"), L"read");

    anna_type_t *readLine_i_history_write_argv[] = {string_type};
    wchar_t *readLine_i_history_write_argn[] = {L"filename"};
    latest_function = anna_module_function(stack, L"write", 0, &readLine_i_history_write, object_type, 1, readLine_i_history_write_argv, readLine_i_history_write_argn, 0, 0);

    anna_type_t *readLine_i_history_append_argv[] = {int_type, string_type};
    wchar_t *readLine_i_history_append_argn[] = {L"count", L"filename"};
    latest_function = anna_module_function(stack, L"append", 0, &readLine_i_history_append, object_type, 2, readLine_i_history_append_argv, readLine_i_history_append_argn, 0, 0);

    anna_type_t *readLine_i_history_truncate_file_argv[] = {string_type, int_type};
    wchar_t *readLine_i_history_truncate_file_argn[] = {L"filename", L"count"};
    latest_function = anna_module_function(stack, L"truncateFile", 0, &readLine_i_history_truncate_file, object_type, 2, readLine_i_history_truncate_file_argv, readLine_i_history_truncate_file_argn, 0, 0);

    anna_type_data_register(anna_history_type_data, stack);
}


// This function is called to create all types defined in this module

void anna_readLine_create(anna_stack_template_t *stack);
void anna_readLine_create(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_readLine_type_data, stack);        
}

// This function is called to load all functions and other declarations into the module

void anna_readLine_load(anna_stack_template_t *stack);
void anna_readLine_load(anna_stack_template_t *stack)
{
    mid_t latest_mid;    
    anna_function_t *latest_function;
    anna_type_t *stack_type = anna_stack_wrap(stack)->type;
    anna_module_data_t modules[] =
        {
            { L"history", anna_history_create, anna_history_load},
        };
    anna_module_data_create(modules, stack);

    wchar_t *this_argn[] = {L"this"};


    anna_type_t *readLine_i__read_line_argv[] = {string_type};
    wchar_t *readLine_i__read_line_argn[] = {L"prompt"};
    latest_function = anna_module_function(stack, L"readLine", 0, &readLine_i__read_line, string_type, 1, readLine_i__read_line_argv, readLine_i__read_line_argn, 0, 0);
    anna_stack_document(stack, L"A wrapper for the GNU readline library.");
    anna_stack_document(stack, L"Currently rather incomplete, only the more basic functions are supported");

    anna_type_data_register(anna_readLine_type_data, stack);
}

