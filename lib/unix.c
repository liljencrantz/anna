
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
#include <sys/wait.h>
#include <grp.h>
#include <stdint.h>
#include <poll.h>
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
anna_type_t *unix_stat_type;
anna_type_t *unix_f_lock_type;
anna_type_t *unix_fd_set_type;
anna_type_t *unix_time_val_type;
anna_type_t *unix_r_limit_type;


// Data used to initialize all types defined in this module
const static anna_type_data_t anna_unix_type_data[] = 
{
};

// This is the source code of the various wrapper functions
const static anna_type_data_t anna_io_type_data[] = 
{
    { &unix_stat_type, L"Stat" },
    { &unix_f_lock_type, L"FLock" },
    { &unix_fd_set_type, L"FdSet" },
    { &unix_time_val_type, L"TimeVal" },
};
const static anna_type_data_t anna_open_mode_type_data[] = 
{
};

void anna_open_mode_create(anna_stack_template_t *stack);
void anna_open_mode_create(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_open_mode_type_data, stack);        
}
void anna_open_mode_load(anna_stack_template_t *stack);
void anna_open_mode_load(anna_stack_template_t *stack)
{
    anna_type_t *stack_type = anna_stack_wrap(stack)->type;
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

     anna_type_data_register(anna_open_mode_type_data, stack);
}
const static anna_type_data_t anna_stat_mode_type_data[] = 
{
};

void anna_stat_mode_create(anna_stack_template_t *stack);
void anna_stat_mode_create(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_stat_mode_type_data, stack);        
}
void anna_stat_mode_load(anna_stack_template_t *stack);
void anna_stat_mode_load(anna_stack_template_t *stack)
{
    anna_type_t *stack_type = anna_stack_wrap(stack)->type;
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

     anna_type_data_register(anna_stat_mode_type_data, stack);
}

ANNA_VM_NATIVE(unix_i_open, 3)
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

ANNA_VM_NATIVE(unix_i_creat, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_name = anna_string_payload_narrow(anna_as_obj(param[0]));
    if(param[1] == null_entry){return null_entry;}
    int native_param_mode = anna_as_int(param[1]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(creat(native_param_name, native_param_mode));
    // Perform cleanup
    free(native_param_name);

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_read, 3)
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

ANNA_VM_NATIVE(unix_i_write, 3)
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

ANNA_VM_NATIVE(unix_i_close, 1)
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

ANNA_VM_NATIVE(unix_i_stat_init, 1)
{
    struct stat *data = (struct stat *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    memset(data, 0, sizeof(struct stat));
    return param[0];
}

ANNA_VM_NATIVE(unix_i_stat_dev_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_dev);
    return result;
}

ANNA_VM_NATIVE(unix_i_stat_ino_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_ino);
    return result;
}

ANNA_VM_NATIVE(unix_i_stat_mode_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_mode);
    return result;
}

ANNA_VM_NATIVE(unix_i_stat_nlink_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_nlink);
    return result;
}

ANNA_VM_NATIVE(unix_i_stat_uid_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_uid);
    return result;
}

ANNA_VM_NATIVE(unix_i_stat_gid_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_gid);
    return result;
}

ANNA_VM_NATIVE(unix_i_stat_rdev_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_rdev);
    return result;
}

ANNA_VM_NATIVE(unix_i_stat_size_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_size);
    return result;
}

ANNA_VM_NATIVE(unix_i_stat_blksize_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_blksize);
    return result;
}

ANNA_VM_NATIVE(unix_i_stat_blocks_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_blocks);
    return result;
}

ANNA_VM_NATIVE(unix_i_stat_atime_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_atime);
    return result;
}

ANNA_VM_NATIVE(unix_i_stat_mtime_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_mtime);
    return result;
}

ANNA_VM_NATIVE(unix_i_stat_ctime_getter, 1)
{
    struct stat *data = (struct stat *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->st_ctime);
    return result;
}

ANNA_VM_NATIVE(unix_i_stat, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_path = anna_string_payload_narrow(anna_as_obj(param[0]));
    if(param[1] == null_entry){return null_entry;}
    struct stat *native_param_buf = (struct stat *)anna_entry_get_addr(anna_as_obj_fast(param[1]), ANNA_MID_CSTRUCT_PAYLOAD);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(stat(native_param_path, native_param_buf));
    // Perform cleanup
    free(native_param_path);

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_lstat, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_path = anna_string_payload_narrow(anna_as_obj(param[0]));
    if(param[1] == null_entry){return null_entry;}
    struct stat *native_param_buf = (struct stat *)anna_entry_get_addr(anna_as_obj_fast(param[1]), ANNA_MID_CSTRUCT_PAYLOAD);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(lstat(native_param_path, native_param_buf));
    // Perform cleanup
    free(native_param_path);

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_fstat, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_fd = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    struct stat *native_param_buf = (struct stat *)anna_entry_get_addr(anna_as_obj_fast(param[1]), ANNA_MID_CSTRUCT_PAYLOAD);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(fstat(native_param_fd, native_param_buf));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_mkdir, 2)
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

ANNA_VM_NATIVE(unix_i_getcwd, 2)
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

ANNA_VM_NATIVE(unix_i_chdir, 1)
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

ANNA_VM_NATIVE(unix_i_fchdir, 1)
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

ANNA_VM_NATIVE(unix_i_f_lock_init, 1)
{
    struct flock *data = (struct flock *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    memset(data, 0, sizeof(struct flock));
    return param[0];
}

ANNA_VM_NATIVE(unix_i_f_lock_type_getter, 1)
{
    struct flock *data = (struct flock *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->l_type);
    return result;
}

ANNA_VM_NATIVE(unix_i_f_lock_whence_getter, 1)
{
    struct flock *data = (struct flock *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->l_whence);
    return result;
}

ANNA_VM_NATIVE(unix_i_f_lock_start_getter, 1)
{
    struct flock *data = (struct flock *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->l_start);
    return result;
}

ANNA_VM_NATIVE(unix_i_f_lock_len_getter, 1)
{
    struct flock *data = (struct flock *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->l_len);
    return result;
}

ANNA_VM_NATIVE(unix_i_f_lock_pid_getter, 1)
{
    struct flock *data = (struct flock *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->l_pid);
    return result;
}
const static anna_type_data_t anna_fcntl_mode_type_data[] = 
{
};

void anna_fcntl_mode_create(anna_stack_template_t *stack);
void anna_fcntl_mode_create(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_fcntl_mode_type_data, stack);        
}
void anna_fcntl_mode_load(anna_stack_template_t *stack);
void anna_fcntl_mode_load(anna_stack_template_t *stack)
{
    anna_type_t *stack_type = anna_stack_wrap(stack)->type;
    anna_module_data_t modules[] =
        {
        };
    anna_module_data_create(modules, stack);

    wchar_t *this_argn[] = {L"this"};

    anna_module_const_int(stack, L"dupFd", F_DUPFD, L"");

     anna_type_data_register(anna_fcntl_mode_type_data, stack);
}
const static anna_type_data_t anna_seek_mode_type_data[] = 
{
};

void anna_seek_mode_create(anna_stack_template_t *stack);
void anna_seek_mode_create(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_seek_mode_type_data, stack);        
}
void anna_seek_mode_load(anna_stack_template_t *stack);
void anna_seek_mode_load(anna_stack_template_t *stack)
{
    anna_type_t *stack_type = anna_stack_wrap(stack)->type;
    anna_module_data_t modules[] =
        {
        };
    anna_module_data_create(modules, stack);

    wchar_t *this_argn[] = {L"this"};

    anna_module_const_int(stack, L"set", SEEK_SET, L"");
    anna_module_const_int(stack, L"cur", SEEK_CUR, L"");
    anna_module_const_int(stack, L"end", SEEK_END, L"");

     anna_type_data_register(anna_seek_mode_type_data, stack);
}

ANNA_VM_NATIVE(unix_i_fcntl, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_fd = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    int native_param_cmd = anna_as_int(param[1]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(fcntl(native_param_fd, native_param_cmd));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_fcntl_int, 3)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_fd = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    int native_param_cmd = anna_as_int(param[1]);
    if(param[2] == null_entry){return null_entry;}
    int native_param_arg = anna_as_int(param[2]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(fcntl(native_param_fd, native_param_cmd, native_param_arg));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_fcntl_f_lock, 3)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_fd = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    int native_param_cmd = anna_as_int(param[1]);
    if(param[2] == null_entry){return null_entry;}
    struct flock *native_param_arg = (struct flock *)anna_entry_get_addr(anna_as_obj_fast(param[2]), ANNA_MID_CSTRUCT_PAYLOAD);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(fcntl(native_param_fd, native_param_cmd, native_param_arg));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_dup, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_fd = anna_as_int(param[0]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(dup(native_param_fd));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_dup2, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_oldfd = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    int native_param_newfd = anna_as_int(param[1]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(dup2(native_param_oldfd, native_param_newfd));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_chown, 3)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_path = anna_string_payload_narrow(anna_as_obj(param[0]));
    if(param[1] == null_entry){return null_entry;}
    int native_param_owner = anna_as_int(param[1]);
    if(param[2] == null_entry){return null_entry;}
    int native_param_group = anna_as_int(param[2]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(chown(native_param_path, native_param_owner, native_param_group));
    // Perform cleanup
    free(native_param_path);

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_fchown, 3)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_fd = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    int native_param_owner = anna_as_int(param[1]);
    if(param[2] == null_entry){return null_entry;}
    int native_param_group = anna_as_int(param[2]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(fchown(native_param_fd, native_param_owner, native_param_group));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_lchown, 3)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_path = anna_string_payload_narrow(anna_as_obj(param[0]));
    if(param[1] == null_entry){return null_entry;}
    int native_param_owner = anna_as_int(param[1]);
    if(param[2] == null_entry){return null_entry;}
    int native_param_group = anna_as_int(param[2]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(lchown(native_param_path, native_param_owner, native_param_group));
    // Perform cleanup
    free(native_param_path);

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_chmod, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_path = anna_string_payload_narrow(anna_as_obj(param[0]));
    if(param[1] == null_entry){return null_entry;}
    int native_param_mode = anna_as_int(param[1]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(chmod(native_param_path, native_param_mode));
    // Perform cleanup
    free(native_param_path);

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_fchmod, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_fd = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    int native_param_mode = anna_as_int(param[1]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(fchmod(native_param_fd, native_param_mode));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_symlink, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_oldpath = anna_string_payload_narrow(anna_as_obj(param[0]));
    if(param[1] == null_entry){return null_entry;}
    char *native_param_newpath = anna_string_payload_narrow(anna_as_obj(param[1]));

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(symlink(native_param_oldpath, native_param_newpath));
    // Perform cleanup
    free(native_param_oldpath);
    free(native_param_newpath);

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_link, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_oldpath = anna_string_payload_narrow(anna_as_obj(param[0]));
    if(param[1] == null_entry){return null_entry;}
    char *native_param_newpath = anna_string_payload_narrow(anna_as_obj(param[1]));

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(link(native_param_oldpath, native_param_newpath));
    // Perform cleanup
    free(native_param_oldpath);
    free(native_param_newpath);

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_unlink, 1)
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

ANNA_VM_NATIVE(unix_i_rmdir, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_path = anna_string_payload_narrow(anna_as_obj(param[0]));

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(rmdir(native_param_path));
    // Perform cleanup
    free(native_param_path);

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_rename, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_oldpath = anna_string_payload_narrow(anna_as_obj(param[0]));
    if(param[1] == null_entry){return null_entry;}
    char *native_param_newpath = anna_string_payload_narrow(anna_as_obj(param[1]));

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(rename(native_param_oldpath, native_param_newpath));
    // Perform cleanup
    free(native_param_oldpath);
    free(native_param_newpath);

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_pipe, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    if(anna_list_ensure_capacity(anna_as_obj(param[0]), 2))
    {
        return null_entry;
    }
    size_t native_param_fd_count = anna_list_get_count(anna_as_obj(param[0]));
    int* native_param_fd = malloc(sizeof(int) * native_param_fd_count);
    if(!native_param_fd){ return null_entry; }
    int native_param_fd_idx;
    for(native_param_fd_idx=0; native_param_fd_idx < native_param_fd_count; native_param_fd_idx++)
    {
        anna_entry_t *tmp = anna_list_get(anna_as_obj(param[0]), native_param_fd_idx);
        if(tmp == null_entry)
        {
            native_param_fd[native_param_fd_idx] = 0;
        }
        else
        {
            int native_param_fd_val = anna_as_int(tmp);
            native_param_fd[native_param_fd_idx] = native_param_fd_val;
        }
    }


    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(pipe(native_param_fd));
    // Perform cleanup
    
    for(native_param_fd_idx=0; native_param_fd_idx < native_param_fd_count; native_param_fd_idx++)
    {
        anna_entry_t *tmp = anna_from_int(native_param_fd[native_param_fd_idx]);
        anna_list_set(anna_as_obj(param[0]), native_param_fd_idx, tmp);
        
    }
    free(native_param_fd);


    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_lseek, 3)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_fd = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    uint64_t native_param_offset = anna_as_int(param[1]);
    if(param[2] == null_entry){return null_entry;}
    int native_param_whence = anna_as_int(param[2]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_uint64(lseek(native_param_fd, native_param_offset, native_param_whence));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_sync, 0)
{
    // Mangle input parameters

    // Validate parameters

    // Call the function
    sync(); anna_entry_t *result = null_entry;
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_fsync, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_fd = anna_as_int(param[0]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(fsync(native_param_fd));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_fdatasync, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_fd = anna_as_int(param[0]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(fdatasync(native_param_fd));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_umask, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_mask = anna_as_int(param[0]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(umask(native_param_mask));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_fd_set_init, 1)
{
    fd_set *data = (fd_set *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    memset(data, 0, sizeof(fd_set));
    return param[0];
}

ANNA_VM_NATIVE(unix_i_time_val_init, 1)
{
    struct timeval *data = (struct timeval *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    memset(data, 0, sizeof(struct timeval));
    return param[0];
}

ANNA_VM_NATIVE(unix_i_time_val_sec_getter, 1)
{
    struct timeval *data = (struct timeval *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->tv_sec);
    return result;
}

ANNA_VM_NATIVE(unix_i_time_val_sec_setter, 2)
{
    struct timeval *data = (struct timeval *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    int tmp = anna_as_int(param[1]);
    data->tv_sec = tmp;
    return param[1];
}

ANNA_VM_NATIVE(unix_i_time_val_usec_getter, 1)
{
    struct timeval *data = (struct timeval *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->tv_usec);
    return result;
}

ANNA_VM_NATIVE(unix_i_time_val_usec_setter, 2)
{
    struct timeval *data = (struct timeval *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    int tmp = anna_as_int(param[1]);
    data->tv_usec = tmp;
    return param[1];
}

ANNA_VM_NATIVE(unix_i_fd_clear, 2)
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

ANNA_VM_NATIVE(unix_i_fd_is_set, 2)
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

ANNA_VM_NATIVE(unix_i_fd_set, 2)
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

ANNA_VM_NATIVE(unix_i_fd_zero, 1)
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

ANNA_VM_NATIVE(unix_i_select, 5)
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

void anna_io_create(anna_stack_template_t *stack);
void anna_io_create(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_io_type_data, stack);        
}
void anna_io_load(anna_stack_template_t *stack);
void anna_io_load(anna_stack_template_t *stack)
{
    anna_type_t *stack_type = anna_stack_wrap(stack)->type;
    anna_module_data_t modules[] =
        {
            { L"openMode", anna_open_mode_create, anna_open_mode_load},
            { L"statMode", anna_stat_mode_create, anna_stat_mode_load},
            { L"fcntlMode", anna_fcntl_mode_create, anna_fcntl_mode_load},
            { L"seekMode", anna_seek_mode_create, anna_seek_mode_load},
        };
    anna_module_data_create(modules, stack);

    wchar_t *this_argn[] = {L"this"};


    anna_type_t *unix_i_open_argv[] = {string_type, int_type, int_type};
    wchar_t *unix_i_open_argn[] = {L"name", L"flags", L"mode"};
    anna_module_function(stack, L"open", 0, &unix_i_open, int_type, 3, unix_i_open_argv, unix_i_open_argn, L"");

    anna_type_t *unix_i_creat_argv[] = {string_type, int_type};
    wchar_t *unix_i_creat_argn[] = {L"name", L"mode"};
    anna_module_function(stack, L"creat", 0, &unix_i_creat, int_type, 2, unix_i_creat_argv, unix_i_creat_argn, L"");

    anna_type_t *unix_i_read_argv[] = {int_type, buffer_type, int_type};
    wchar_t *unix_i_read_argn[] = {L"fd", L"buffer", L"count"};
    anna_module_function(stack, L"read", 0, &unix_i_read, int_type, 3, unix_i_read_argv, unix_i_read_argn, L"");

    anna_type_t *unix_i_write_argv[] = {int_type, buffer_type, int_type};
    wchar_t *unix_i_write_argn[] = {L"fd", L"buffer", L"count"};
    anna_module_function(stack, L"write", 0, &unix_i_write, int_type, 3, unix_i_write_argv, unix_i_write_argn, L"");

    anna_type_t *unix_i_close_argv[] = {int_type};
    wchar_t *unix_i_close_argn[] = {L"fd"};
    anna_module_function(stack, L"close", 0, &unix_i_close, int_type, 1, unix_i_close_argv, unix_i_close_argn, L"");

    anna_member_create_blob(unix_stat_type, ANNA_MID_CSTRUCT_PAYLOAD, 0, sizeof(struct stat));

    anna_member_create_native_method(
	unix_stat_type, anna_mid_get(L"__init__"), 0,
	&unix_i_stat_init, object_type, 1, &unix_stat_type, this_argn, 0, 0);    

    anna_member_create_native_property(
        unix_stat_type, anna_mid_get(L"dev"),
        int_type, unix_i_stat_dev_getter, 0, 0);

    anna_member_create_native_property(
        unix_stat_type, anna_mid_get(L"ino"),
        int_type, unix_i_stat_ino_getter, 0, 0);

    anna_member_create_native_property(
        unix_stat_type, anna_mid_get(L"mode"),
        int_type, unix_i_stat_mode_getter, 0, 0);

    anna_member_create_native_property(
        unix_stat_type, anna_mid_get(L"nlink"),
        int_type, unix_i_stat_nlink_getter, 0, 0);

    anna_member_create_native_property(
        unix_stat_type, anna_mid_get(L"uid"),
        int_type, unix_i_stat_uid_getter, 0, 0);

    anna_member_create_native_property(
        unix_stat_type, anna_mid_get(L"gid"),
        int_type, unix_i_stat_gid_getter, 0, 0);

    anna_member_create_native_property(
        unix_stat_type, anna_mid_get(L"rdev"),
        int_type, unix_i_stat_rdev_getter, 0, 0);

    anna_member_create_native_property(
        unix_stat_type, anna_mid_get(L"size"),
        int_type, unix_i_stat_size_getter, 0, 0);

    anna_member_create_native_property(
        unix_stat_type, anna_mid_get(L"blksize"),
        int_type, unix_i_stat_blksize_getter, 0, 0);

    anna_member_create_native_property(
        unix_stat_type, anna_mid_get(L"blocks"),
        int_type, unix_i_stat_blocks_getter, 0, 0);

    anna_member_create_native_property(
        unix_stat_type, anna_mid_get(L"atime"),
        int_type, unix_i_stat_atime_getter, 0, 0);

    anna_member_create_native_property(
        unix_stat_type, anna_mid_get(L"mtime"),
        int_type, unix_i_stat_mtime_getter, 0, 0);

    anna_member_create_native_property(
        unix_stat_type, anna_mid_get(L"ctime"),
        int_type, unix_i_stat_ctime_getter, 0, 0);

    anna_type_t *unix_i_stat_argv[] = {string_type, unix_stat_type};
    wchar_t *unix_i_stat_argn[] = {L"path", L"buf"};
    anna_module_function(stack, L"stat", 0, &unix_i_stat, int_type, 2, unix_i_stat_argv, unix_i_stat_argn, L"");

    anna_type_t *unix_i_lstat_argv[] = {string_type, unix_stat_type};
    wchar_t *unix_i_lstat_argn[] = {L"path", L"buf"};
    anna_module_function(stack, L"lstat", 0, &unix_i_lstat, int_type, 2, unix_i_lstat_argv, unix_i_lstat_argn, L"");

    anna_type_t *unix_i_fstat_argv[] = {int_type, unix_stat_type};
    wchar_t *unix_i_fstat_argn[] = {L"fd", L"buf"};
    anna_module_function(stack, L"fstat", 0, &unix_i_fstat, int_type, 2, unix_i_fstat_argv, unix_i_fstat_argn, L"");

    anna_type_t *unix_i_mkdir_argv[] = {string_type, int_type};
    wchar_t *unix_i_mkdir_argn[] = {L"path", L"mode"};
    anna_module_function(stack, L"mkdir", 0, &unix_i_mkdir, int_type, 2, unix_i_mkdir_argv, unix_i_mkdir_argn, L"");
    anna_module_const_int(stack, L"standardInput", 0, L"");
    anna_module_const_int(stack, L"standardOutput", 1, L"");
    anna_module_const_int(stack, L"standardError", 2, L"");

    anna_type_t *unix_i_getcwd_argv[] = {buffer_type, int_type};
    wchar_t *unix_i_getcwd_argn[] = {L"buf", L"size"};
    anna_module_function(stack, L"getcwd", 0, &unix_i_getcwd, int_type, 2, unix_i_getcwd_argv, unix_i_getcwd_argn, L"");

    anna_type_t *unix_i_chdir_argv[] = {string_type};
    wchar_t *unix_i_chdir_argn[] = {L"path"};
    anna_module_function(stack, L"chdir", 0, &unix_i_chdir, int_type, 1, unix_i_chdir_argv, unix_i_chdir_argn, L"");

    anna_type_t *unix_i_fchdir_argv[] = {int_type};
    wchar_t *unix_i_fchdir_argn[] = {L"fd"};
    anna_module_function(stack, L"fchdir", 0, &unix_i_fchdir, int_type, 1, unix_i_fchdir_argv, unix_i_fchdir_argn, L"");

    anna_member_create_blob(unix_f_lock_type, ANNA_MID_CSTRUCT_PAYLOAD, 0, sizeof(struct flock));

    anna_member_create_native_method(
	unix_f_lock_type, anna_mid_get(L"__init__"), 0,
	&unix_i_f_lock_init, object_type, 1, &unix_f_lock_type, this_argn, 0, 0);    

    anna_member_create_native_property(
        unix_f_lock_type, anna_mid_get(L"type"),
        int_type, unix_i_f_lock_type_getter, 0, 0);

    anna_member_create_native_property(
        unix_f_lock_type, anna_mid_get(L"whence"),
        int_type, unix_i_f_lock_whence_getter, 0, 0);

    anna_member_create_native_property(
        unix_f_lock_type, anna_mid_get(L"start"),
        int_type, unix_i_f_lock_start_getter, 0, 0);

    anna_member_create_native_property(
        unix_f_lock_type, anna_mid_get(L"len"),
        int_type, unix_i_f_lock_len_getter, 0, 0);

    anna_member_create_native_property(
        unix_f_lock_type, anna_mid_get(L"pid"),
        int_type, unix_i_f_lock_pid_getter, 0, 0);

    anna_type_t *unix_i_fcntl_argv[] = {int_type, int_type};
    wchar_t *unix_i_fcntl_argn[] = {L"fd", L"cmd"};
    anna_module_function(stack, L"fcntl", 0, &unix_i_fcntl, int_type, 2, unix_i_fcntl_argv, unix_i_fcntl_argn, L"");

    anna_type_t *unix_i_fcntl_int_argv[] = {int_type, int_type, int_type};
    wchar_t *unix_i_fcntl_int_argn[] = {L"fd", L"cmd", L"arg"};
    anna_module_function(stack, L"fcntlInt", 0, &unix_i_fcntl_int, int_type, 3, unix_i_fcntl_int_argv, unix_i_fcntl_int_argn, L"");

    anna_type_t *unix_i_fcntl_f_lock_argv[] = {int_type, int_type, unix_f_lock_type};
    wchar_t *unix_i_fcntl_f_lock_argn[] = {L"fd", L"cmd", L"arg"};
    anna_module_function(stack, L"fcntlFLock", 0, &unix_i_fcntl_f_lock, int_type, 3, unix_i_fcntl_f_lock_argv, unix_i_fcntl_f_lock_argn, L"");

    anna_type_t *unix_i_dup_argv[] = {int_type};
    wchar_t *unix_i_dup_argn[] = {L"fd"};
    anna_module_function(stack, L"dup", 0, &unix_i_dup, int_type, 1, unix_i_dup_argv, unix_i_dup_argn, L"");

    anna_type_t *unix_i_dup2_argv[] = {int_type, int_type};
    wchar_t *unix_i_dup2_argn[] = {L"oldfd", L"newfd"};
    anna_module_function(stack, L"dup2", 0, &unix_i_dup2, int_type, 2, unix_i_dup2_argv, unix_i_dup2_argn, L"");

    anna_type_t *unix_i_chown_argv[] = {string_type, int_type, int_type};
    wchar_t *unix_i_chown_argn[] = {L"path", L"owner", L"group"};
    anna_module_function(stack, L"chown", 0, &unix_i_chown, int_type, 3, unix_i_chown_argv, unix_i_chown_argn, L"");

    anna_type_t *unix_i_fchown_argv[] = {int_type, int_type, int_type};
    wchar_t *unix_i_fchown_argn[] = {L"fd", L"owner", L"group"};
    anna_module_function(stack, L"fchown", 0, &unix_i_fchown, int_type, 3, unix_i_fchown_argv, unix_i_fchown_argn, L"");

    anna_type_t *unix_i_lchown_argv[] = {string_type, int_type, int_type};
    wchar_t *unix_i_lchown_argn[] = {L"path", L"owner", L"group"};
    anna_module_function(stack, L"lchown", 0, &unix_i_lchown, int_type, 3, unix_i_lchown_argv, unix_i_lchown_argn, L"");

    anna_type_t *unix_i_chmod_argv[] = {string_type, int_type};
    wchar_t *unix_i_chmod_argn[] = {L"path", L"mode"};
    anna_module_function(stack, L"chmod", 0, &unix_i_chmod, int_type, 2, unix_i_chmod_argv, unix_i_chmod_argn, L"");

    anna_type_t *unix_i_fchmod_argv[] = {int_type, int_type};
    wchar_t *unix_i_fchmod_argn[] = {L"fd", L"mode"};
    anna_module_function(stack, L"fchmod", 0, &unix_i_fchmod, int_type, 2, unix_i_fchmod_argv, unix_i_fchmod_argn, L"");

    anna_type_t *unix_i_symlink_argv[] = {string_type, string_type};
    wchar_t *unix_i_symlink_argn[] = {L"oldpath", L"newpath"};
    anna_module_function(stack, L"symlink", 0, &unix_i_symlink, int_type, 2, unix_i_symlink_argv, unix_i_symlink_argn, L"");

    anna_type_t *unix_i_link_argv[] = {string_type, string_type};
    wchar_t *unix_i_link_argn[] = {L"oldpath", L"newpath"};
    anna_module_function(stack, L"link", 0, &unix_i_link, int_type, 2, unix_i_link_argv, unix_i_link_argn, L"");

    anna_type_t *unix_i_unlink_argv[] = {string_type};
    wchar_t *unix_i_unlink_argn[] = {L"path"};
    anna_module_function(stack, L"unlink", 0, &unix_i_unlink, int_type, 1, unix_i_unlink_argv, unix_i_unlink_argn, L"");

    anna_type_t *unix_i_rmdir_argv[] = {string_type};
    wchar_t *unix_i_rmdir_argn[] = {L"path"};
    anna_module_function(stack, L"rmdir", 0, &unix_i_rmdir, int_type, 1, unix_i_rmdir_argv, unix_i_rmdir_argn, L"");

    anna_type_t *unix_i_rename_argv[] = {string_type, string_type};
    wchar_t *unix_i_rename_argn[] = {L"oldpath", L"newpath"};
    anna_module_function(stack, L"rename", 0, &unix_i_rename, int_type, 2, unix_i_rename_argv, unix_i_rename_argn, L"");

    anna_type_t *unix_i_pipe_argv[] = {anna_list_type_get_mutable(int_type)};
    wchar_t *unix_i_pipe_argn[] = {L"fd"};
    anna_module_function(stack, L"pipe", 0, &unix_i_pipe, int_type, 1, unix_i_pipe_argv, unix_i_pipe_argn, L"");

    anna_type_t *unix_i_lseek_argv[] = {int_type, int_type, int_type};
    wchar_t *unix_i_lseek_argn[] = {L"fd", L"offset", L"whence"};
    anna_module_function(stack, L"lseek", 0, &unix_i_lseek, int_type, 3, unix_i_lseek_argv, unix_i_lseek_argn, L"");

    anna_type_t *unix_i_sync_argv[] = {};
    wchar_t *unix_i_sync_argn[] = {};
    anna_module_function(stack, L"sync", 0, &unix_i_sync, object_type, 0, unix_i_sync_argv, unix_i_sync_argn, L"");

    anna_type_t *unix_i_fsync_argv[] = {int_type};
    wchar_t *unix_i_fsync_argn[] = {L"fd"};
    anna_module_function(stack, L"fsync", 0, &unix_i_fsync, int_type, 1, unix_i_fsync_argv, unix_i_fsync_argn, L"");

    anna_type_t *unix_i_fdatasync_argv[] = {int_type};
    wchar_t *unix_i_fdatasync_argn[] = {L"fd"};
    anna_module_function(stack, L"fdatasync", 0, &unix_i_fdatasync, int_type, 1, unix_i_fdatasync_argv, unix_i_fdatasync_argn, L"");

    anna_type_t *unix_i_umask_argv[] = {int_type};
    wchar_t *unix_i_umask_argn[] = {L"mask"};
    anna_module_function(stack, L"umask", 0, &unix_i_umask, int_type, 1, unix_i_umask_argv, unix_i_umask_argn, L"");

    anna_member_create_blob(unix_fd_set_type, ANNA_MID_CSTRUCT_PAYLOAD, 0, sizeof(fd_set));

    anna_member_create_native_method(
	unix_fd_set_type, anna_mid_get(L"__init__"), 0,
	&unix_i_fd_set_init, object_type, 1, &unix_fd_set_type, this_argn, 0, 0);    

    anna_member_create_blob(unix_time_val_type, ANNA_MID_CSTRUCT_PAYLOAD, 0, sizeof(struct timeval));

    anna_member_create_native_method(
	unix_time_val_type, anna_mid_get(L"__init__"), 0,
	&unix_i_time_val_init, object_type, 1, &unix_time_val_type, this_argn, 0, 0);    

    anna_member_create_native_property(
        unix_time_val_type, anna_mid_get(L"sec"),
        int_type, unix_i_time_val_sec_getter, unix_i_time_val_sec_setter, 0);

    anna_member_create_native_property(
        unix_time_val_type, anna_mid_get(L"usec"),
        int_type, unix_i_time_val_usec_getter, unix_i_time_val_usec_setter, 0);

    anna_type_t *unix_i_fd_clear_argv[] = {int_type, unix_fd_set_type};
    wchar_t *unix_i_fd_clear_argn[] = {L"fd", L"set"};
    anna_module_function(stack, L"fdClear", 0, &unix_i_fd_clear, object_type, 2, unix_i_fd_clear_argv, unix_i_fd_clear_argn, L"");

    anna_type_t *unix_i_fd_is_set_argv[] = {int_type, unix_fd_set_type};
    wchar_t *unix_i_fd_is_set_argn[] = {L"fd", L"set"};
    anna_module_function(stack, L"fdIsSet", 0, &unix_i_fd_is_set, int_type, 2, unix_i_fd_is_set_argv, unix_i_fd_is_set_argn, L"");

    anna_type_t *unix_i_fd_set_argv[] = {int_type, unix_fd_set_type};
    wchar_t *unix_i_fd_set_argn[] = {L"fd", L"set"};
    anna_module_function(stack, L"fdSet", 0, &unix_i_fd_set, object_type, 2, unix_i_fd_set_argv, unix_i_fd_set_argn, L"");

    anna_type_t *unix_i_fd_zero_argv[] = {unix_fd_set_type};
    wchar_t *unix_i_fd_zero_argn[] = {L"set"};
    anna_module_function(stack, L"fdZero", 0, &unix_i_fd_zero, object_type, 1, unix_i_fd_zero_argv, unix_i_fd_zero_argn, L"");

    anna_type_t *unix_i_select_argv[] = {int_type, unix_fd_set_type, unix_fd_set_type, unix_fd_set_type, unix_time_val_type};
    wchar_t *unix_i_select_argn[] = {L"nfds", L"readfds", L"writefds", L"exceptfds", L"timeout"};
    anna_module_function(stack, L"select", 0, &unix_i_select, int_type, 5, unix_i_select_argv, unix_i_select_argn, L"");

     anna_type_data_register(anna_io_type_data, stack);
}
const static anna_type_data_t anna_proc_type_data[] = 
{
};

ANNA_VM_NATIVE(unix_i_exec, 3)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_filename = anna_string_payload_narrow(anna_as_obj(param[0]));
    if(param[1] == null_entry){return null_entry;}
    size_t native_param_argv_count = anna_list_get_count(anna_as_obj(param[1]));
    char ** native_param_argv = malloc(sizeof(char *) * native_param_argv_count);
    if(!native_param_argv){ return null_entry; }
    int native_param_argv_idx;
    for(native_param_argv_idx=0; native_param_argv_idx < native_param_argv_count; native_param_argv_idx++)
    {
        char *native_param_argv_val = anna_string_payload_narrow(anna_as_obj(anna_list_get(anna_as_obj(param[1]), native_param_argv_idx)));
        native_param_argv[native_param_argv_idx] = native_param_argv_val;
    }

    if(param[2] == null_entry){return null_entry;}
    size_t native_param_envp_count = anna_list_get_count(anna_as_obj(param[2]));
    char ** native_param_envp = malloc(sizeof(char *) * native_param_envp_count);
    if(!native_param_envp){ return null_entry; }
    int native_param_envp_idx;
    for(native_param_envp_idx=0; native_param_envp_idx < native_param_envp_count; native_param_envp_idx++)
    {
        char *native_param_envp_val = anna_string_payload_narrow(anna_as_obj(anna_list_get(anna_as_obj(param[2]), native_param_envp_idx)));
        native_param_envp[native_param_envp_idx] = native_param_envp_val;
    }


    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(execve(native_param_filename, native_param_argv, native_param_envp));
    // Perform cleanup
    free(native_param_filename);
    
    for(native_param_argv_idx=0; native_param_argv_idx < native_param_argv_count; native_param_argv_idx++)
    {
        free(native_param_argv[native_param_argv_idx]);
    }
    free(native_param_argv);

    
    for(native_param_envp_idx=0; native_param_envp_idx < native_param_envp_count; native_param_envp_idx++)
    {
        free(native_param_envp[native_param_envp_idx]);
    }
    free(native_param_envp);


    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_exit, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_status = anna_as_int(param[0]);

    // Validate parameters

    // Call the function
    exit(native_param_status); anna_entry_t *result = null_entry;
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_fork, 0)
{
    // Mangle input parameters

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(fork());
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_kill, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_pid = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    int native_param_sig = anna_as_int(param[1]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(kill(native_param_pid, native_param_sig));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_getsid, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_pid = anna_as_int(param[0]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(getsid(native_param_pid));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_setsid, 0)
{
    // Mangle input parameters

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(setsid());
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_getpid, 0)
{
    // Mangle input parameters

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(getpid());
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_getppid, 0)
{
    // Mangle input parameters

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(getppid());
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_wait, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    if(anna_list_ensure_capacity(anna_as_obj(param[0]), 1))
    {
        return null_entry;
    }
    size_t native_param_status_count = anna_list_get_count(anna_as_obj(param[0]));
    int* native_param_status = malloc(sizeof(int) * native_param_status_count);
    if(!native_param_status){ return null_entry; }
    int native_param_status_idx;
    for(native_param_status_idx=0; native_param_status_idx < native_param_status_count; native_param_status_idx++)
    {
        anna_entry_t *tmp = anna_list_get(anna_as_obj(param[0]), native_param_status_idx);
        if(tmp == null_entry)
        {
            native_param_status[native_param_status_idx] = 0;
        }
        else
        {
            int native_param_status_val = anna_as_int(tmp);
            native_param_status[native_param_status_idx] = native_param_status_val;
        }
    }


    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(wait(native_param_status));
    // Perform cleanup
    
    for(native_param_status_idx=0; native_param_status_idx < native_param_status_count; native_param_status_idx++)
    {
        anna_entry_t *tmp = anna_from_int(native_param_status[native_param_status_idx]);
        anna_list_set(anna_as_obj(param[0]), native_param_status_idx, tmp);
        
    }
    free(native_param_status);


    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_waitpid, 3)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_pid = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    if(anna_list_ensure_capacity(anna_as_obj(param[1]), 1))
    {
        return null_entry;
    }
    size_t native_param_status_count = anna_list_get_count(anna_as_obj(param[1]));
    int* native_param_status = malloc(sizeof(int) * native_param_status_count);
    if(!native_param_status){ return null_entry; }
    int native_param_status_idx;
    for(native_param_status_idx=0; native_param_status_idx < native_param_status_count; native_param_status_idx++)
    {
        anna_entry_t *tmp = anna_list_get(anna_as_obj(param[1]), native_param_status_idx);
        if(tmp == null_entry)
        {
            native_param_status[native_param_status_idx] = 0;
        }
        else
        {
            int native_param_status_val = anna_as_int(tmp);
            native_param_status[native_param_status_idx] = native_param_status_val;
        }
    }

    if(param[2] == null_entry){return null_entry;}
    int native_param_options = anna_as_int(param[2]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(waitpid(native_param_pid, native_param_status, native_param_options));
    // Perform cleanup
    
    for(native_param_status_idx=0; native_param_status_idx < native_param_status_count; native_param_status_idx++)
    {
        anna_entry_t *tmp = anna_from_int(native_param_status[native_param_status_idx]);
        anna_list_set(anna_as_obj(param[1]), native_param_status_idx, tmp);
        
    }
    free(native_param_status);


    // Return result
    return result;
}

void anna_proc_create(anna_stack_template_t *stack);
void anna_proc_create(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_proc_type_data, stack);        
}
void anna_proc_load(anna_stack_template_t *stack);
void anna_proc_load(anna_stack_template_t *stack)
{
    anna_type_t *stack_type = anna_stack_wrap(stack)->type;
    anna_module_data_t modules[] =
        {
        };
    anna_module_data_create(modules, stack);

    wchar_t *this_argn[] = {L"this"};


    anna_type_t *unix_i_exec_argv[] = {string_type, anna_list_type_get_any(string_type), anna_list_type_get_any(string_type)};
    wchar_t *unix_i_exec_argn[] = {L"filename", L"argv", L"envp"};
    anna_module_function(stack, L"exec", 0, &unix_i_exec, int_type, 3, unix_i_exec_argv, unix_i_exec_argn, L"");

    anna_type_t *unix_i_exit_argv[] = {int_type};
    wchar_t *unix_i_exit_argn[] = {L"status"};
    anna_module_function(stack, L"exit", 0, &unix_i_exit, object_type, 1, unix_i_exit_argv, unix_i_exit_argn, L"");

    anna_type_t *unix_i_fork_argv[] = {};
    wchar_t *unix_i_fork_argn[] = {};
    anna_module_function(stack, L"fork", 0, &unix_i_fork, int_type, 0, unix_i_fork_argv, unix_i_fork_argn, L"");

    anna_type_t *unix_i_kill_argv[] = {int_type, int_type};
    wchar_t *unix_i_kill_argn[] = {L"pid", L"sig"};
    anna_module_function(stack, L"kill", 0, &unix_i_kill, int_type, 2, unix_i_kill_argv, unix_i_kill_argn, L"");

    anna_type_t *unix_i_getsid_argv[] = {int_type};
    wchar_t *unix_i_getsid_argn[] = {L"pid"};
    anna_module_function(stack, L"getsid", 0, &unix_i_getsid, int_type, 1, unix_i_getsid_argv, unix_i_getsid_argn, L"");

    anna_type_t *unix_i_setsid_argv[] = {};
    wchar_t *unix_i_setsid_argn[] = {};
    anna_module_function(stack, L"setsid", 0, &unix_i_setsid, int_type, 0, unix_i_setsid_argv, unix_i_setsid_argn, L"");

    anna_type_t *unix_i_getpid_argv[] = {};
    wchar_t *unix_i_getpid_argn[] = {};
    anna_module_function(stack, L"getpid", 0, &unix_i_getpid, int_type, 0, unix_i_getpid_argv, unix_i_getpid_argn, L"");

    anna_type_t *unix_i_getppid_argv[] = {};
    wchar_t *unix_i_getppid_argn[] = {};
    anna_module_function(stack, L"getppid", 0, &unix_i_getppid, int_type, 0, unix_i_getppid_argv, unix_i_getppid_argn, L"");

    anna_type_t *unix_i_wait_argv[] = {anna_list_type_get_mutable(int_type)};
    wchar_t *unix_i_wait_argn[] = {L"status"};
    anna_module_function(stack, L"wait", 0, &unix_i_wait, int_type, 1, unix_i_wait_argv, unix_i_wait_argn, L"");

    anna_type_t *unix_i_waitpid_argv[] = {int_type, anna_list_type_get_mutable(int_type), int_type};
    wchar_t *unix_i_waitpid_argn[] = {L"pid", L"status", L"options"};
    anna_module_function(stack, L"waitpid", 0, &unix_i_waitpid, int_type, 3, unix_i_waitpid_argv, unix_i_waitpid_argn, L"");

     anna_type_data_register(anna_proc_type_data, stack);
}
const static anna_type_data_t anna_user_type_data[] = 
{
};

ANNA_VM_NATIVE(unix_i_getuid, 0)
{
    // Mangle input parameters

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(getuid());
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_geteuid, 0)
{
    // Mangle input parameters

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(geteuid());
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_getgid, 0)
{
    // Mangle input parameters

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(getgid());
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_getegid, 0)
{
    // Mangle input parameters

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(getegid());
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_setuid, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_uid = anna_as_int(param[0]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(setuid(native_param_uid));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_seteuid, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_uid = anna_as_int(param[0]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(seteuid(native_param_uid));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_setegid, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_uid = anna_as_int(param[0]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(setegid(native_param_uid));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_setgid, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_uid = anna_as_int(param[0]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(setgid(native_param_uid));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_setpgid, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_pid = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    int native_param_pgid = anna_as_int(param[1]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(setpgid(native_param_pid, native_param_pgid));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_getpgid, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_pid = anna_as_int(param[0]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(getpgid(native_param_pid));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_getgroups, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_size = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    if(anna_list_ensure_capacity(anna_as_obj(param[1]), native_param_size))
    {
        return null_entry;
    }
    size_t native_param_list_count = anna_list_get_count(anna_as_obj(param[1]));
    int* native_param_list = malloc(sizeof(int) * native_param_list_count);
    if(!native_param_list){ return null_entry; }
    int native_param_list_idx;
    for(native_param_list_idx=0; native_param_list_idx < native_param_list_count; native_param_list_idx++)
    {
        anna_entry_t *tmp = anna_list_get(anna_as_obj(param[1]), native_param_list_idx);
        if(tmp == null_entry)
        {
            native_param_list[native_param_list_idx] = 0;
        }
        else
        {
            int native_param_list_val = anna_as_int(tmp);
            native_param_list[native_param_list_idx] = native_param_list_val;
        }
    }


    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(getgroups(native_param_size, native_param_list));
    // Perform cleanup
    
    for(native_param_list_idx=0; native_param_list_idx < native_param_list_count; native_param_list_idx++)
    {
        anna_entry_t *tmp = anna_from_int(native_param_list[native_param_list_idx]);
        anna_list_set(anna_as_obj(param[1]), native_param_list_idx, tmp);
        
    }
    free(native_param_list);


    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_setgroups, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_size = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    if(anna_list_ensure_capacity(anna_as_obj(param[1]), native_param_size))
    {
        return null_entry;
    }
    size_t native_param_list_count = anna_list_get_count(anna_as_obj(param[1]));
    int* native_param_list = malloc(sizeof(int) * native_param_list_count);
    if(!native_param_list){ return null_entry; }
    int native_param_list_idx;
    for(native_param_list_idx=0; native_param_list_idx < native_param_list_count; native_param_list_idx++)
    {
        anna_entry_t *tmp = anna_list_get(anna_as_obj(param[1]), native_param_list_idx);
        if(tmp == null_entry)
        {
            native_param_list[native_param_list_idx] = 0;
        }
        else
        {
            int native_param_list_val = anna_as_int(tmp);
            native_param_list[native_param_list_idx] = native_param_list_val;
        }
    }


    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(setgroups(native_param_size, native_param_list));
    // Perform cleanup
    
    for(native_param_list_idx=0; native_param_list_idx < native_param_list_count; native_param_list_idx++)
    {
        anna_entry_t *tmp = anna_from_int(native_param_list[native_param_list_idx]);
        anna_list_set(anna_as_obj(param[1]), native_param_list_idx, tmp);
        
    }
    free(native_param_list);


    // Return result
    return result;
}

void anna_user_create(anna_stack_template_t *stack);
void anna_user_create(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_user_type_data, stack);        
}
void anna_user_load(anna_stack_template_t *stack);
void anna_user_load(anna_stack_template_t *stack)
{
    anna_type_t *stack_type = anna_stack_wrap(stack)->type;
    anna_module_data_t modules[] =
        {
        };
    anna_module_data_create(modules, stack);

    wchar_t *this_argn[] = {L"this"};


    anna_type_t *unix_i_getuid_argv[] = {};
    wchar_t *unix_i_getuid_argn[] = {};
    anna_module_function(stack, L"getuid", 0, &unix_i_getuid, int_type, 0, unix_i_getuid_argv, unix_i_getuid_argn, L"");

    anna_type_t *unix_i_geteuid_argv[] = {};
    wchar_t *unix_i_geteuid_argn[] = {};
    anna_module_function(stack, L"geteuid", 0, &unix_i_geteuid, int_type, 0, unix_i_geteuid_argv, unix_i_geteuid_argn, L"");

    anna_type_t *unix_i_getgid_argv[] = {};
    wchar_t *unix_i_getgid_argn[] = {};
    anna_module_function(stack, L"getgid", 0, &unix_i_getgid, int_type, 0, unix_i_getgid_argv, unix_i_getgid_argn, L"");

    anna_type_t *unix_i_getegid_argv[] = {};
    wchar_t *unix_i_getegid_argn[] = {};
    anna_module_function(stack, L"getegid", 0, &unix_i_getegid, int_type, 0, unix_i_getegid_argv, unix_i_getegid_argn, L"");

    anna_type_t *unix_i_setuid_argv[] = {int_type};
    wchar_t *unix_i_setuid_argn[] = {L"uid"};
    anna_module_function(stack, L"setuid", 0, &unix_i_setuid, int_type, 1, unix_i_setuid_argv, unix_i_setuid_argn, L"");

    anna_type_t *unix_i_seteuid_argv[] = {int_type};
    wchar_t *unix_i_seteuid_argn[] = {L"uid"};
    anna_module_function(stack, L"seteuid", 0, &unix_i_seteuid, int_type, 1, unix_i_seteuid_argv, unix_i_seteuid_argn, L"");

    anna_type_t *unix_i_setegid_argv[] = {int_type};
    wchar_t *unix_i_setegid_argn[] = {L"uid"};
    anna_module_function(stack, L"setegid", 0, &unix_i_setegid, int_type, 1, unix_i_setegid_argv, unix_i_setegid_argn, L"");

    anna_type_t *unix_i_setgid_argv[] = {int_type};
    wchar_t *unix_i_setgid_argn[] = {L"uid"};
    anna_module_function(stack, L"setgid", 0, &unix_i_setgid, int_type, 1, unix_i_setgid_argv, unix_i_setgid_argn, L"");

    anna_type_t *unix_i_setpgid_argv[] = {int_type, int_type};
    wchar_t *unix_i_setpgid_argn[] = {L"pid", L"pgid"};
    anna_module_function(stack, L"setpgid", 0, &unix_i_setpgid, int_type, 2, unix_i_setpgid_argv, unix_i_setpgid_argn, L"");

    anna_type_t *unix_i_getpgid_argv[] = {int_type};
    wchar_t *unix_i_getpgid_argn[] = {L"pid"};
    anna_module_function(stack, L"getpgid", 0, &unix_i_getpgid, int_type, 1, unix_i_getpgid_argv, unix_i_getpgid_argn, L"");

    anna_type_t *unix_i_getgroups_argv[] = {int_type, anna_list_type_get_mutable(int_type)};
    wchar_t *unix_i_getgroups_argn[] = {L"size", L"list"};
    anna_module_function(stack, L"getgroups", 0, &unix_i_getgroups, int_type, 2, unix_i_getgroups_argv, unix_i_getgroups_argn, L"");

    anna_type_t *unix_i_setgroups_argv[] = {int_type, anna_list_type_get_mutable(int_type)};
    wchar_t *unix_i_setgroups_argn[] = {L"size", L"list"};
    anna_module_function(stack, L"setgroups", 0, &unix_i_setgroups, int_type, 2, unix_i_setgroups_argv, unix_i_setgroups_argn, L"");

     anna_type_data_register(anna_user_type_data, stack);
}
const static anna_type_data_t anna_r_limit_type_data[] = 
{
    { &unix_r_limit_type, L"RLimit" },
};
const static anna_type_data_t anna_r_limit_mode_type_data[] = 
{
};

void anna_r_limit_mode_create(anna_stack_template_t *stack);
void anna_r_limit_mode_create(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_r_limit_mode_type_data, stack);        
}
void anna_r_limit_mode_load(anna_stack_template_t *stack);
void anna_r_limit_mode_load(anna_stack_template_t *stack)
{
    anna_type_t *stack_type = anna_stack_wrap(stack)->type;
    anna_module_data_t modules[] =
        {
        };
    anna_module_data_create(modules, stack);

    wchar_t *this_argn[] = {L"this"};

    anna_module_const_int(stack, L"as", RLIMIT_AS, L"");
    anna_module_const_int(stack, L"core", RLIMIT_CORE, L"");
    anna_module_const_int(stack, L"cpu", RLIMIT_CPU, L"");
    anna_module_const_int(stack, L"data", RLIMIT_DATA, L"");
    anna_module_const_int(stack, L"fsize", RLIMIT_FSIZE, L"");
    anna_module_const_int(stack, L"memlock", RLIMIT_MEMLOCK, L"");

     anna_type_data_register(anna_r_limit_mode_type_data, stack);
}

ANNA_VM_NATIVE(unix_i_r_limit_init, 1)
{
    struct rlimit *data = (struct rlimit *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    memset(data, 0, sizeof(struct rlimit));
    return param[0];
}

ANNA_VM_NATIVE(unix_i_r_limit_cur_getter, 1)
{
    struct rlimit *data = (struct rlimit *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->rlim_cur);
    return result;
}

ANNA_VM_NATIVE(unix_i_r_limit_max_getter, 1)
{
    struct rlimit *data = (struct rlimit *)anna_entry_get_addr(anna_as_obj_fast(param[0]), ANNA_MID_CSTRUCT_PAYLOAD);
    anna_entry_t *result = anna_from_int(data->rlim_max);
    return result;
}

ANNA_VM_NATIVE(unix_i_get_r_limit, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_resource = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    struct rlimit *native_param_rlim = (struct rlimit *)anna_entry_get_addr(anna_as_obj_fast(param[1]), ANNA_MID_CSTRUCT_PAYLOAD);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(getrlimit(native_param_resource, native_param_rlim));
    // Perform cleanup

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_set_r_limit, 2)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    int native_param_resource = anna_as_int(param[0]);
    if(param[1] == null_entry){return null_entry;}
    struct rlimit *native_param_rlim = (struct rlimit *)anna_entry_get_addr(anna_as_obj_fast(param[1]), ANNA_MID_CSTRUCT_PAYLOAD);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(setrlimit(native_param_resource, native_param_rlim));
    // Perform cleanup

    // Return result
    return result;
}

void anna_r_limit_create(anna_stack_template_t *stack);
void anna_r_limit_create(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_r_limit_type_data, stack);        
}
void anna_r_limit_load(anna_stack_template_t *stack);
void anna_r_limit_load(anna_stack_template_t *stack)
{
    anna_type_t *stack_type = anna_stack_wrap(stack)->type;
    anna_module_data_t modules[] =
        {
            { L"rLimitMode", anna_r_limit_mode_create, anna_r_limit_mode_load},
        };
    anna_module_data_create(modules, stack);

    wchar_t *this_argn[] = {L"this"};


    anna_member_create_blob(unix_r_limit_type, ANNA_MID_CSTRUCT_PAYLOAD, 0, sizeof(struct rlimit));

    anna_member_create_native_method(
	unix_r_limit_type, anna_mid_get(L"__init__"), 0,
	&unix_i_r_limit_init, object_type, 1, &unix_r_limit_type, this_argn, 0, 0);    

    anna_member_create_native_property(
        unix_r_limit_type, anna_mid_get(L"cur"),
        int_type, unix_i_r_limit_cur_getter, 0, 0);

    anna_member_create_native_property(
        unix_r_limit_type, anna_mid_get(L"max"),
        int_type, unix_i_r_limit_max_getter, 0, 0);

    anna_type_t *unix_i_get_r_limit_argv[] = {int_type, unix_r_limit_type};
    wchar_t *unix_i_get_r_limit_argn[] = {L"resource", L"rlim"};
    anna_module_function(stack, L"getRLimit", 0, &unix_i_get_r_limit, int_type, 2, unix_i_get_r_limit_argv, unix_i_get_r_limit_argn, L"");

    anna_type_t *unix_i_set_r_limit_argv[] = {int_type, unix_r_limit_type};
    wchar_t *unix_i_set_r_limit_argn[] = {L"resource", L"rlim"};
    anna_module_function(stack, L"setRLimit", 0, &unix_i_set_r_limit, int_type, 2, unix_i_set_r_limit_argv, unix_i_set_r_limit_argn, L"");

     anna_type_data_register(anna_r_limit_type_data, stack);
}
const static anna_type_data_t anna_env_type_data[] = 
{
};

ANNA_VM_NATIVE(unix_i_getenv, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_name = anna_string_payload_narrow(anna_as_obj(param[0]));

    // Validate parameters

    // Call the function
    anna_entry_t *result = (getenv(native_param_name)) ? anna_from_obj(anna_string_create_narrow(strlen(getenv(native_param_name)), getenv(native_param_name))) : null_entry;
    // Perform cleanup
    free(native_param_name);

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_setenv, 3)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_name = anna_string_payload_narrow(anna_as_obj(param[0]));
    if(param[1] == null_entry){return null_entry;}
    char *native_param_value = anna_string_payload_narrow(anna_as_obj(param[1]));
    if(param[2] == null_entry){return null_entry;}
    int native_param_overwrite = anna_as_int(param[2]);

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(setenv(native_param_name, native_param_value, native_param_overwrite));
    // Perform cleanup
    free(native_param_name);
    free(native_param_value);

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_unsetenv, 1)
{
    // Mangle input parameters
    if(param[0] == null_entry){return null_entry;}
    char *native_param_name = anna_string_payload_narrow(anna_as_obj(param[0]));

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(unsetenv(native_param_name));
    // Perform cleanup
    free(native_param_name);

    // Return result
    return result;
}

ANNA_VM_NATIVE(unix_i_clearenv, 0)
{
    // Mangle input parameters

    // Validate parameters

    // Call the function
    anna_entry_t *result = anna_from_int(clearenv());
    // Perform cleanup

    // Return result
    return result;
}

void anna_env_create(anna_stack_template_t *stack);
void anna_env_create(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_env_type_data, stack);        
}
void anna_env_load(anna_stack_template_t *stack);
void anna_env_load(anna_stack_template_t *stack)
{
    anna_type_t *stack_type = anna_stack_wrap(stack)->type;
    anna_module_data_t modules[] =
        {
        };
    anna_module_data_create(modules, stack);

    wchar_t *this_argn[] = {L"this"};


    anna_type_t *unix_i_getenv_argv[] = {string_type};
    wchar_t *unix_i_getenv_argn[] = {L"name"};
    anna_module_function(stack, L"getenv", 0, &unix_i_getenv, string_type, 1, unix_i_getenv_argv, unix_i_getenv_argn, L"");

    anna_type_t *unix_i_setenv_argv[] = {string_type, string_type, int_type};
    wchar_t *unix_i_setenv_argn[] = {L"name", L"value", L"overwrite"};
    anna_module_function(stack, L"setenv", 0, &unix_i_setenv, int_type, 3, unix_i_setenv_argv, unix_i_setenv_argn, L"");

    anna_type_t *unix_i_unsetenv_argv[] = {string_type};
    wchar_t *unix_i_unsetenv_argn[] = {L"name"};
    anna_module_function(stack, L"unsetenv", 0, &unix_i_unsetenv, int_type, 1, unix_i_unsetenv_argv, unix_i_unsetenv_argn, L"");

    anna_type_t *unix_i_clearenv_argv[] = {};
    wchar_t *unix_i_clearenv_argn[] = {};
    anna_module_function(stack, L"clearenv", 0, &unix_i_clearenv, int_type, 0, unix_i_clearenv_argv, unix_i_clearenv_argn, L"");
    anna_module_const_int(stack, L"environ", environ, L"");

     anna_type_data_register(anna_env_type_data, stack);
}


// This function is called to create all types defined in this module

void anna_unix_create(anna_stack_template_t *stack);
void anna_unix_create(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_unix_type_data, stack);        
}

// This function is called to load all functions and other declarations into the module

void anna_unix_load(anna_stack_template_t *stack);
void anna_unix_load(anna_stack_template_t *stack)
{
    anna_type_t *stack_type = anna_stack_wrap(stack)->type;
    anna_module_data_t modules[] =
        {
            { L"io", anna_io_create, anna_io_load},
            { L"proc", anna_proc_create, anna_proc_load},
            { L"user", anna_user_create, anna_user_load},
            { L"rLimit", anna_r_limit_create, anna_r_limit_load},
            { L"env", anna_env_create, anna_env_load},
        };
    anna_module_data_create(modules, stack);

    wchar_t *this_argn[] = {L"this"};


     anna_type_data_register(anna_unix_type_data, stack);
}

