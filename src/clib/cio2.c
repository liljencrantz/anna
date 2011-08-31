
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "common.h"
#include "util.h"
#include "anna.h"
#include "anna_mid.h"
#include "anna_type.h"
#include "anna_type_data.h"
#include "anna_vm.h"
#include "anna_module.h"
#include "clib/lang/string.h"
#include "clib/lang/buffer.h"
#include "anna_member.h"
#include "anna_module_data.h"

// Declare internal variables for all types defined in this module
anna_type_t *cio2_stat_type;

// Data used to initialize all types defined in this module
const static anna_type_data_t anna_type_data[] = 
{
    { &cio2_stat_type, L"Stat" },
};

// This is the source code of the various wrapper functions

ANNA_VM_NATIVE(cio2_i_stat_init, 1)
{
    struct stat *data = (struct stat *)anna_entry_get(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    memset(data, 0, sizeof(struct stat));
    return param[0];
}

ANNA_VM_NATIVE(cio2_i_stat_dev_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_dev);
    return result;
}

ANNA_VM_NATIVE(cio2_i_stat_ino_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_ino);
    return result;
}

ANNA_VM_NATIVE(cio2_i_stat_mode_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_mode);
    return result;
}

ANNA_VM_NATIVE(cio2_i_stat_nlink_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_nlink);
    return result;
}

ANNA_VM_NATIVE(cio2_i_stat_uid_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_uid);
    return result;
}

ANNA_VM_NATIVE(cio2_i_stat_gid_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_gid);
    return result;
}

ANNA_VM_NATIVE(cio2_i_stat_rdev_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_rdev);
    return result;
}

ANNA_VM_NATIVE(cio2_i_stat_size_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_size);
    return result;
}

ANNA_VM_NATIVE(cio2_i_stat_blksize_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_blksize);
    return result;
}

ANNA_VM_NATIVE(cio2_i_stat_blocks_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_blocks);
    return result;
}

ANNA_VM_NATIVE(cio2_i_stat_atime_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_atime);
    return result;
}

ANNA_VM_NATIVE(cio2_i_stat_mtime_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_mtime);
    return result;
}

ANNA_VM_NATIVE(cio2_i_stat_ctime_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_ctime);
    return result;
}

void anna_open_mode_load(anna_stack_template_t *stack)
{
    anna_module_data_t modules[] =
        {
        };
    anna_module_data_create(modules, stack);

    wchar_t *this_argn[] = {L"this"};

    anna_module_const_int(stack, L"readOnly", O_RDONLY, L"");
    anna_module_const_int(stack, L"writeOnly", O_WRONLY, L"");
    anna_module_const_int(stack, L"readWrite", O_RDWR, L"");
    anna_module_const_int(stack, L"append", O_APPEND, L"");
    anna_module_const_int(stack, L"async", O_ASYNC, L"");
    anna_module_const_int(stack, L"create", O_CREAT, L"");
    anna_module_const_int(stack, L"closeOnExec", O_CLOEXEC, L"");
    anna_module_const_int(stack, L"direct", O_DIRECT, L"");
    anna_module_const_int(stack, L"directory", O_DIRECTORY, L"");
    anna_module_const_int(stack, L"exclusive", O_EXCL, L"");
    anna_module_const_int(stack, L"largeFile", O_LARGEFILE, L"");
    anna_module_const_int(stack, L"noAccessTime", O_NOATIME, L"");
    anna_module_const_int(stack, L"noControllingTTY", O_NOCTTY, L"");
    anna_module_const_int(stack, L"noFollow", O_NOFOLLOW, L"");
    anna_module_const_int(stack, L"nonBlock", O_NONBLOCK, L"");
    anna_module_const_int(stack, L"synchronous", O_SYNC, L"");
    anna_module_const_int(stack, L"truncate", O_TRUNC, L"");


}

void anna_stat_mode_load(anna_stack_template_t *stack)
{
    anna_module_data_t modules[] =
        {
        };
    anna_module_data_create(modules, stack);

    wchar_t *this_argn[] = {L"this"};

    anna_module_const_int(stack, L"regular", S_IFREG, L"");
    anna_module_const_int(stack, L"socket", S_IFSOCK, L"");
    anna_module_const_int(stack, L"link", S_IFLNK, L"");
    anna_module_const_int(stack, L"block", S_IFBLK, L"");
    anna_module_const_int(stack, L"directory", S_IFDIR, L"");
    anna_module_const_int(stack, L"character", S_IFCHR, L"");
    anna_module_const_int(stack, L"fifo", S_IFIFO, L"");
    anna_module_const_int(stack, L"suid", S_ISUID, L"");
    anna_module_const_int(stack, L"sgid", S_ISGID, L"");
    anna_module_const_int(stack, L"sticky", S_ISVTX, L"");
    anna_module_const_int(stack, L"userAll", S_IRWXU, L"");
    anna_module_const_int(stack, L"userRead", S_IRUSR, L"");
    anna_module_const_int(stack, L"userwrite", S_IWUSR, L"");
    anna_module_const_int(stack, L"userExecute", S_IXUSR, L"");
    anna_module_const_int(stack, L"groupAll", S_IRWXG, L"");
    anna_module_const_int(stack, L"groupRead", S_IRGRP, L"");
    anna_module_const_int(stack, L"groupwrite", S_IWGRP, L"");
    anna_module_const_int(stack, L"groupExecute", S_IXGRP, L"");
    anna_module_const_int(stack, L"otherAll", S_IRWXO, L"");
    anna_module_const_int(stack, L"otherRead", S_IROTH, L"");
    anna_module_const_int(stack, L"otherwrite", S_IWOTH, L"");
    anna_module_const_int(stack, L"otherExecute", S_IXOTH, L"");


}

ANNA_VM_NATIVE(cio2_i_open, 3)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_name = anna_string_payload_narrow(anna_as_obj(param[0]));
    if(param[1] == null_entry){return null_entry;}
    int native_param_flags = anna_as_int(param[1]);
    if(param[2] == null_entry){return null_entry;}
    int native_param_mode = anna_as_int(param[2]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(open(native_param_name, native_param_flags, native_param_mode));
    // Perform cleanup
    free(native_param_name);

    // Return result
    return result;
}

ANNA_VM_NATIVE(cio2_i_read, 3)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_fd = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    unsigned char *native_param_buffer = anna_buffer_get_payload(anna_as_obj(param[1]));
    if(param[2] == null_entry){return null_entry;}
    int native_param_count = anna_as_int(param[2]);

    // Validate parameters
    if(anna_buffer_ensure_capacity(anna_as_obj(param[1]), native_param_count)) 
    {
        return null_entry;
    }
    else
    {
        native_param_buffer = anna_buffer_get_payload(anna_as_obj(param[1]));
    }

    // Call the function
    anna_entry_t *result = anna_from_int(read(native_param_fd, native_param_buffer, native_param_count));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(cio2_i_write, 3)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_fd = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    unsigned char *native_param_buffer = anna_buffer_get_payload(anna_as_obj(param[1]));
    if(param[2] == null_entry){return null_entry;}
    int native_param_count = anna_as_int(param[2]);

    // Validate parameters
    if(anna_buffer_ensure_capacity(anna_as_obj(param[1]), native_param_count)) 
    {
        return null_entry;
    }
    else
    {
        native_param_buffer = anna_buffer_get_payload(anna_as_obj(param[1]));
    }

    // Call the function
    anna_entry_t *result = anna_from_int(write(native_param_fd, native_param_buffer, native_param_count));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(cio2_i_close, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_fd = anna_as_int(param[0]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(close(native_param_fd));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(cio2_i_stat, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_path = anna_string_payload_narrow(anna_as_obj(param[0]));
    if(param[1] == null_entry){return null_entry;}
    struct stat *native_param_buf = (struct stat *)anna_entry_get(anna_as_obj_fast(param[1]), ANNA_MID_CSTRUCT_PAYLOAD);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(stat(native_param_path, native_param_buf));
    // Perform cleanup
    free(native_param_path);

    // Return result
    return result;
}

ANNA_VM_NATIVE(cio2_i_lstat, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_path = anna_string_payload_narrow(anna_as_obj(param[0]));
    if(param[1] == null_entry){return null_entry;}
    struct stat *native_param_buf = (struct stat *)anna_entry_get(anna_as_obj_fast(param[1]), ANNA_MID_CSTRUCT_PAYLOAD);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(lstat(native_param_path, native_param_buf));
    // Perform cleanup
    free(native_param_path);

    // Return result
    return result;
}

ANNA_VM_NATIVE(cio2_i_fstat, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_fd = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    struct stat *native_param_buf = (struct stat *)anna_entry_get(anna_as_obj_fast(param[1]), ANNA_MID_CSTRUCT_PAYLOAD);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(fstat(native_param_fd, native_param_buf));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(cio2_i_mkdir, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_path = anna_string_payload_narrow(anna_as_obj(param[0]));
    if(param[1] == null_entry){return null_entry;}
    int native_param_mode = anna_as_int(param[1]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(mkdir(native_param_path, native_param_mode));
    // Perform cleanup
    free(native_param_path);

    // Return result
    return result;
}

ANNA_VM_NATIVE(cio2_i_unlink, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_path = anna_string_payload_narrow(anna_as_obj(param[0]));

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(unlink(native_param_path));
    // Perform cleanup
    free(native_param_path);

    // Return result
    return result;
}

ANNA_VM_NATIVE(cio2_i_getcwd, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    unsigned char *native_param_buf = anna_buffer_get_payload(anna_as_obj(param[0]));
    if(param[1] == null_entry){return null_entry;}
    int native_param_size = anna_as_int(param[1]);

    // Validate parameters
    if(anna_buffer_ensure_capacity(anna_as_obj(param[0]), native_param_size)) 
    {
        return null_entry;
    }
    else
    {
        native_param_buf = anna_buffer_get_payload(anna_as_obj(param[0]));
    }

    // Call the function
    anna_entry_t *result = anna_from_int(getcwd(native_param_buf, native_param_size));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(cio2_i_chdir, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_path = anna_string_payload_narrow(anna_as_obj(param[0]));

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(chdir(native_param_path));
    // Perform cleanup
    free(native_param_path);

    // Return result
    return result;
}

ANNA_VM_NATIVE(cio2_i_fchdir, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_fd = anna_as_int(param[0]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(fchdir(native_param_fd));
    // Perform cleanup

    // Return result
    return result;
}


// This function is called to create all types defined in this module
void create(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_type_data, stack);        
}

// This function is called to load all functions and other declarations into the module

void load(anna_stack_template_t *stack)
{
    anna_module_data_t modules[] =
        {
            { L"openMode", 0, anna_open_mode_load},
            { L"statMode", 0, anna_stat_mode_load},
        };
    anna_module_data_create(modules, stack);

    wchar_t *this_argn[] = {L"this"};


    anna_member_create_blob(cio2_stat_type, ANNA_MID_CSTRUCT_PAYLOAD, 0, sizeof(struct stat));

    anna_member_create_native_method(
	cio2_stat_type, anna_mid_get(L"__init__"), 0,
	&cio2_i_stat_init, object_type, 1, &cio2_stat_type, this_argn);    

    anna_member_create_native_property(
        cio2_stat_type, anna_mid_get(L"dev"),
        int_type, cio2_i_stat_dev_getter, 0, 0);

    anna_member_create_native_property(
        cio2_stat_type, anna_mid_get(L"ino"),
        int_type, cio2_i_stat_ino_getter, 0, 0);

    anna_member_create_native_property(
        cio2_stat_type, anna_mid_get(L"mode"),
        int_type, cio2_i_stat_mode_getter, 0, 0);

    anna_member_create_native_property(
        cio2_stat_type, anna_mid_get(L"nlink"),
        int_type, cio2_i_stat_nlink_getter, 0, 0);

    anna_member_create_native_property(
        cio2_stat_type, anna_mid_get(L"uid"),
        int_type, cio2_i_stat_uid_getter, 0, 0);

    anna_member_create_native_property(
        cio2_stat_type, anna_mid_get(L"gid"),
        int_type, cio2_i_stat_gid_getter, 0, 0);

    anna_member_create_native_property(
        cio2_stat_type, anna_mid_get(L"rdev"),
        int_type, cio2_i_stat_rdev_getter, 0, 0);

    anna_member_create_native_property(
        cio2_stat_type, anna_mid_get(L"size"),
        int_type, cio2_i_stat_size_getter, 0, 0);

    anna_member_create_native_property(
        cio2_stat_type, anna_mid_get(L"blksize"),
        int_type, cio2_i_stat_blksize_getter, 0, 0);

    anna_member_create_native_property(
        cio2_stat_type, anna_mid_get(L"blocks"),
        int_type, cio2_i_stat_blocks_getter, 0, 0);

    anna_member_create_native_property(
        cio2_stat_type, anna_mid_get(L"atime"),
        int_type, cio2_i_stat_atime_getter, 0, 0);

    anna_member_create_native_property(
        cio2_stat_type, anna_mid_get(L"mtime"),
        int_type, cio2_i_stat_mtime_getter, 0, 0);

    anna_member_create_native_property(
        cio2_stat_type, anna_mid_get(L"ctime"),
        int_type, cio2_i_stat_ctime_getter, 0, 0);

    anna_type_t *cio2_i_open_argv[] = {string_type, int_type, int_type};
    wchar_t *cio2_i_open_argn[] = {L"name", L"flags", L"mode"};
    anna_module_function(stack, L"open", 0, &cio2_i_open, int_type, 3, cio2_i_open_argv, cio2_i_open_argn, L"");

    anna_type_t *cio2_i_read_argv[] = {int_type, buffer_type, int_type};
    wchar_t *cio2_i_read_argn[] = {L"fd", L"buffer", L"count"};
    anna_module_function(stack, L"read", 0, &cio2_i_read, int_type, 3, cio2_i_read_argv, cio2_i_read_argn, L"");

    anna_type_t *cio2_i_write_argv[] = {int_type, buffer_type, int_type};
    wchar_t *cio2_i_write_argn[] = {L"fd", L"buffer", L"count"};
    anna_module_function(stack, L"write", 0, &cio2_i_write, int_type, 3, cio2_i_write_argv, cio2_i_write_argn, L"");

    anna_type_t *cio2_i_close_argv[] = {int_type};
    wchar_t *cio2_i_close_argn[] = {L"fd"};
    anna_module_function(stack, L"close", 0, &cio2_i_close, int_type, 1, cio2_i_close_argv, cio2_i_close_argn, L"");

    anna_type_t *cio2_i_stat_argv[] = {string_type, cio2_stat_type};
    wchar_t *cio2_i_stat_argn[] = {L"path", L"buf"};
    anna_module_function(stack, L"stat", 0, &cio2_i_stat, int_type, 2, cio2_i_stat_argv, cio2_i_stat_argn, L"");

    anna_type_t *cio2_i_lstat_argv[] = {string_type, cio2_stat_type};
    wchar_t *cio2_i_lstat_argn[] = {L"path", L"buf"};
    anna_module_function(stack, L"lstat", 0, &cio2_i_lstat, int_type, 2, cio2_i_lstat_argv, cio2_i_lstat_argn, L"");

    anna_type_t *cio2_i_fstat_argv[] = {int_type, cio2_stat_type};
    wchar_t *cio2_i_fstat_argn[] = {L"fd", L"buf"};
    anna_module_function(stack, L"fstat", 0, &cio2_i_fstat, int_type, 2, cio2_i_fstat_argv, cio2_i_fstat_argn, L"");

    anna_type_t *cio2_i_mkdir_argv[] = {string_type, int_type};
    wchar_t *cio2_i_mkdir_argn[] = {L"path", L"mode"};
    anna_module_function(stack, L"mkdir", 0, &cio2_i_mkdir, int_type, 2, cio2_i_mkdir_argv, cio2_i_mkdir_argn, L"");

    anna_type_t *cio2_i_unlink_argv[] = {string_type};
    wchar_t *cio2_i_unlink_argn[] = {L"path"};
    anna_module_function(stack, L"unlink", 0, &cio2_i_unlink, int_type, 1, cio2_i_unlink_argv, cio2_i_unlink_argn, L"");
    anna_module_const_int(stack, L"standardInput", 0, L"");
    anna_module_const_int(stack, L"standardOutput", 1, L"");
    anna_module_const_int(stack, L"standardError", 2, L"");

    anna_type_t *cio2_i_getcwd_argv[] = {buffer_type, int_type};
    wchar_t *cio2_i_getcwd_argn[] = {L"buf", L"size"};
    anna_module_function(stack, L"getcwd", 0, &cio2_i_getcwd, int_type, 2, cio2_i_getcwd_argv, cio2_i_getcwd_argn, L"");

    anna_type_t *cio2_i_chdir_argv[] = {string_type};
    wchar_t *cio2_i_chdir_argn[] = {L"path"};
    anna_module_function(stack, L"chdir", 0, &cio2_i_chdir, int_type, 1, cio2_i_chdir_argv, cio2_i_chdir_argn, L"");

    anna_type_t *cio2_i_fchdir_argv[] = {int_type};
    wchar_t *cio2_i_fchdir_argn[] = {L"fd"};
    anna_module_function(stack, L"fchdir", 0, &cio2_i_fchdir, int_type, 1, cio2_i_fchdir_argv, cio2_i_fchdir_argn, L"");

    anna_type_data_register(anna_type_data, stack);
}

