
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>
#include <sys/select.h>

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
anna_type_t *test_fd_set_type;
anna_type_t *test_time_val_type;


// Data used to initialize all types defined in this module
const static anna_type_data_t anna_test_type_data[] = 
{
    { &test_fd_set_type, L"FdSet" },
    { &test_time_val_type, L"TimeVal" },
};

// This is the source code of the various wrapper functions

ANNA_VM_NATIVE(test_i_fd_set_init, 1)
{
    fd_set *data = (fd_set *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    memset(data, 0, sizeof(fd_set));
    return param[0];
}

ANNA_VM_NATIVE(test_i_time_val_init, 1)
{
    struct timeval *data = (struct timeval *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    memset(data, 0, sizeof(struct timeval));
    return param[0];
}

ANNA_VM_NATIVE(test_i_time_val_sec_getter, 1)
{
    struct timeval *data = (struct timeval *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->tv_sec);
    return result;
}

ANNA_VM_NATIVE(test_i_time_val_sec_setter, 2)
{
    struct timeval *data = (struct timeval *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    int tmp = anna_as_int(param[1]);
    data->tv_sec = tmp;
    return param[1];
}

ANNA_VM_NATIVE(test_i_time_val_usec_getter, 1)
{
    struct timeval *data = (struct timeval *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->tv_usec);
    return result;
}

ANNA_VM_NATIVE(test_i_time_val_usec_setter, 2)
{
    struct timeval *data = (struct timeval *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    int tmp = anna_as_int(param[1]);
    data->tv_usec = tmp;
    return param[1];
}

ANNA_VM_NATIVE(test_i_fd_clear, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_fd = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    fd_set *native_param_set = (fd_set *)anna_entry_get_addr(anna_as_obj_fast(param[1]), ANNA_MID_CSTRUCT_PAYLOAD);

    // Validate parameters

    // Call the function
    FD_CLR(native_param_fd, native_param_set); anna_entry_t *result = null_entry;
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(test_i_fd_is_set, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_fd = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    fd_set *native_param_set = (fd_set *)anna_entry_get_addr(anna_as_obj_fast(param[1]), ANNA_MID_CSTRUCT_PAYLOAD);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(FD_ISSET(native_param_fd, native_param_set));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(test_i_fd_set, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_fd = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    fd_set *native_param_set = (fd_set *)anna_entry_get_addr(anna_as_obj_fast(param[1]), ANNA_MID_CSTRUCT_PAYLOAD);

    // Validate parameters

    // Call the function
    FD_SET(native_param_fd, native_param_set); anna_entry_t *result = null_entry;
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(test_i_fd_zero, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    fd_set *native_param_set = (fd_set *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);

    // Validate parameters

    // Call the function
    FD_ZERO(native_param_set); anna_entry_t *result = null_entry;
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(test_i_select, 5)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_nfds = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    fd_set *native_param_readfds = (fd_set *)anna_entry_get_addr(anna_as_obj_fast(param[1]), ANNA_MID_CSTRUCT_PAYLOAD);
    if(param[2] == null_entry){return null_entry;}
    fd_set *native_param_writefds = (fd_set *)anna_entry_get_addr(anna_as_obj_fast(param[2]), ANNA_MID_CSTRUCT_PAYLOAD);
    if(param[3] == null_entry){return null_entry;}
    fd_set *native_param_exceptfds = (fd_set *)anna_entry_get_addr(anna_as_obj_fast(param[3]), ANNA_MID_CSTRUCT_PAYLOAD);
    if(param[4] == null_entry){return null_entry;}
    struct timeval *native_param_timeout = (struct timeval *)anna_entry_get_addr(anna_as_obj_fast(param[4]), ANNA_MID_CSTRUCT_PAYLOAD);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(select(native_param_nfds, native_param_readfds, native_param_writefds, native_param_exceptfds, native_param_timeout));
    // Perform cleanup

    // Return result
    return result;
}


// This function is called to create all types defined in this module

void anna_test_create(anna_stack_template_t *stack);
void anna_test_create(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_test_type_data, stack);        
}

// This function is called to load all functions and other declarations into the module

void anna_test_load(anna_stack_template_t *stack);
void anna_test_load(anna_stack_template_t *stack)
{
    anna_type_t *stack_type = anna_stack_wrap(stack)->type;
    anna_module_data_t modules[] =
        {
        };
    anna_module_data_create(modules, stack);

    wchar_t *this_argn[] = {L"this"};


    anna_member_create_blob(test_fd_set_type, ANNA_MID_CSTRUCT_PAYLOAD, 0, sizeof(fd_set));

    anna_member_create_native_method(
	test_fd_set_type, anna_mid_get(L"__init__"), 0,
	&test_i_fd_set_init, object_type, 1, &test_fd_set_type, this_argn);    

    anna_member_create_blob(test_time_val_type, ANNA_MID_CSTRUCT_PAYLOAD, 0, sizeof(struct timeval));

    anna_member_create_native_method(
	test_time_val_type, anna_mid_get(L"__init__"), 0,
	&test_i_time_val_init, object_type, 1, &test_time_val_type, this_argn);    

    anna_member_create_native_property(
        test_time_val_type, anna_mid_get(L"sec"),
        int_type, test_i_time_val_sec_getter, test_i_time_val_sec_setter, 0);

    anna_member_create_native_property(
        test_time_val_type, anna_mid_get(L"usec"),
        int_type, test_i_time_val_usec_getter, test_i_time_val_usec_setter, 0);

    anna_type_t *test_i_fd_clear_argv[] = {int_type, test_fd_set_type};
    wchar_t *test_i_fd_clear_argn[] = {L"fd", L"set"};
    anna_module_function(stack, L"fdClear", 0, &test_i_fd_clear, object_type, 2, test_i_fd_clear_argv, test_i_fd_clear_argn, L"");

    anna_type_t *test_i_fd_is_set_argv[] = {int_type, test_fd_set_type};
    wchar_t *test_i_fd_is_set_argn[] = {L"fd", L"set"};
    anna_module_function(stack, L"fdIsSet", 0, &test_i_fd_is_set, int_type, 2, test_i_fd_is_set_argv, test_i_fd_is_set_argn, L"");

    anna_type_t *test_i_fd_set_argv[] = {int_type, test_fd_set_type};
    wchar_t *test_i_fd_set_argn[] = {L"fd", L"set"};
    anna_module_function(stack, L"fdSet", 0, &test_i_fd_set, object_type, 2, test_i_fd_set_argv, test_i_fd_set_argn, L"");

    anna_type_t *test_i_fd_zero_argv[] = {test_fd_set_type};
    wchar_t *test_i_fd_zero_argn[] = {L"set"};
    anna_module_function(stack, L"fdZero", 0, &test_i_fd_zero, object_type, 1, test_i_fd_zero_argv, test_i_fd_zero_argn, L"");

    anna_type_t *test_i_select_argv[] = {int_type, test_fd_set_type, test_fd_set_type, test_fd_set_type, test_time_val_type};
    wchar_t *test_i_select_argn[] = {L"nfds", L"readfds", L"writefds", L"exceptfds", L"timeout"};
    anna_module_function(stack, L"select", 0, &test_i_select, int_type, 5, test_i_select_argv, test_i_select_argn, L"");

     anna_type_data_register(anna_test_type_data, stack);
}

