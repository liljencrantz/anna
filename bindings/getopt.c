
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <unistd.h>

#include "common.h"
#include "util.h"
#include "anna.h"
#include "anna_mid.h"
#include "anna_type.h"
#include "anna_type_data.h"
#include "anna_vm.h"
#include "anna_module.h"
#include "clib/lang/list.h"
#include "clib/lang/string.h"
#include "clib/lang/buffer.h"
#include "anna_member.h"
#include "anna_module_data.h"

// Declare internal variables for all types defined in this module


// Data used to initialize all types defined in this module
const static anna_type_data_t anna_getopt_type_data[] = 
{
};

// This is the source code of the various wrapper functions

ANNA_VM_NATIVE(anna_getopt_optarg_getter, 1)
{
    anna_entry_t *result = (optarg) ? anna_from_obj(anna_string_create_narrow(strlen(optarg), optarg)) : null_entry;
    return result;
}

ANNA_VM_NATIVE(anna_getopt_optarg_setter, 2)
{
    if(param[1] != null_entry)
    {
        char *value = anna_string_payload_narrow(anna_as_obj(param[1]));
        optarg = value;
    }
    return param[1];
}

ANNA_VM_NATIVE(anna_getopt_optind_getter, 1)
{
    anna_entry_t *result = anna_from_int(optind);
    return result;
}

ANNA_VM_NATIVE(anna_getopt_optind_setter, 2)
{
    if(param[1] != null_entry)
    {
        int value = anna_as_int(param[1]);
        optind = value;
    }
    return param[1];
}

ANNA_VM_NATIVE(anna_getopt_optopt_getter, 1)
{
    anna_entry_t *result = anna_from_int(optopt);
    return result;
}

ANNA_VM_NATIVE(anna_getopt_optopt_setter, 2)
{
    if(param[1] != null_entry)
    {
        int value = anna_as_int(param[1]);
        optopt = value;
    }
    return param[1];
}

ANNA_VM_NATIVE(anna_getopt_opterr_getter, 1)
{
    anna_entry_t *result = anna_from_int(opterr);
    return result;
}

ANNA_VM_NATIVE(anna_getopt_opterr_setter, 2)
{
    if(param[1] != null_entry)
    {
        int value = anna_as_int(param[1]);
        opterr = value;
    }
    return param[1];
}

ANNA_VM_NATIVE(getopt_i_getopt, 3)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_argc = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    size_t native_param_argv_count = anna_list_get_count(anna_as_obj(param[1]));
    char ** native_param_argv = malloc(sizeof(char *) * native_param_argv_count);
    if(!native_param_argv){ return null_entry; }
    int native_param_argv_idx;
    for(native_param_argv_idx=0; native_param_argv_idx < native_param_argv_count; native_param_argv_idx++)
    {
        anna_entry_t *tmp = anna_list_get(anna_as_obj(param[1]), native_param_argv_idx);
        if(tmp == null_entry)
        {
            native_param_argv[native_param_argv_idx] = 0;
        }
        else
        {
            char *native_param_argv_val = anna_string_payload_narrow(anna_as_obj(tmp));
            native_param_argv[native_param_argv_idx] = native_param_argv_val;
        }
    }

    if(param[2] == null_entry){return null_entry;}
    char *native_param_options = anna_string_payload_narrow(anna_as_obj(param[2]));

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(getopt(native_param_argc, native_param_argv, native_param_options));
    // Perform cleanup
    
    for(native_param_argv_idx=0; native_param_argv_idx < native_param_argv_count; native_param_argv_idx++)
    {
        anna_entry_t *tmp = (native_param_argv[native_param_argv_idx]) ? anna_from_obj(anna_string_create_narrow(strlen(native_param_argv[native_param_argv_idx]), native_param_argv[native_param_argv_idx])) : null_entry;
        anna_list_set(anna_as_obj(param[1]), native_param_argv_idx, tmp);
        free(native_param_argv[native_param_argv_idx]);
    }
    free(native_param_argv);

    free(native_param_options);

    // Return result
    return result;
}


// This function is called to create all types defined in this module

void anna_getopt_create(anna_stack_template_t *stack);
void anna_getopt_create(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_getopt_type_data, stack);        
}

// This function is called to load all functions and other declarations into the module

void anna_getopt_load(anna_stack_template_t *stack);
void anna_getopt_load(anna_stack_template_t *stack)
{
    anna_type_t *stack_type = anna_stack_wrap(stack)->type;
    anna_module_data_t modules[] =
        {
        };
    anna_module_data_create(modules, stack);

    wchar_t *this_argn[] = {L"this"};

    anna_member_create_native_property(
        stack_type, anna_mid_get(L"optarg"),
        string_type, anna_getopt_optarg_getter, anna_getopt_optarg_setter, 0);
    anna_member_create_native_property(
        stack_type, anna_mid_get(L"optind"),
        int_type, anna_getopt_optind_getter, anna_getopt_optind_setter, 0);
    anna_member_create_native_property(
        stack_type, anna_mid_get(L"optopt"),
        int_type, anna_getopt_optopt_getter, anna_getopt_optopt_setter, 0);
    anna_member_create_native_property(
        stack_type, anna_mid_get(L"opterr"),
        int_type, anna_getopt_opterr_getter, anna_getopt_opterr_setter, 0);

    anna_type_t *getopt_i_getopt_argv[] = {int_type, anna_list_type_get_mutable(string_type), string_type};
    wchar_t *getopt_i_getopt_argn[] = {L"argc", L"argv", L"options"};
    anna_module_function(stack, L"getopt", 0, &getopt_i_getopt, int_type, 3, getopt_i_getopt_argv, getopt_i_getopt_argn, L"");

     anna_type_data_register(anna_getopt_type_data, stack);
}

