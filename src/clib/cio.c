#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "common.h"
#include "util.h"
#include "wutil.h"
#include "anna.h"
#include "anna_module.h"
#include "anna_module_data.h"
#include "anna_stack.h"
#include "anna_vm.h"
#include "anna_intern.h"
#include "anna_member.h"
#include "anna_mid.h"
#include "anna_function.h"
#include "anna_type.h"

#include "clib/clib.h"
#include "clib/lang/string.h"
#include "clib/lang/buffer.h"
#include "clib/lang/list.h"

#define READ_SZ 4096

ANNA_VM_NATIVE(anna_cio_open, 3)
{
    if(anna_entry_null(param[0]) || anna_entry_null(param[1]) || anna_entry_null(param[2]))
    {
	return null_entry;
    }
    
    wchar_t *nam = anna_string_payload(anna_as_obj(param[0]));
    int flags = anna_as_int(param[1]);
    int mode = anna_as_int(param[2]);
    int res = wopen(nam, flags, mode);

    if(res == -1)
    {
	return null_entry;
    }
    return anna_from_int(res);
}

ANNA_VM_NATIVE(anna_cio_read, 3)
{
    if(anna_entry_null(param[0]) || anna_entry_null(param[1]))
    {
	return null_entry;
    }
    
    int fd = anna_as_int(param[0]);
    anna_object_t *buff = anna_as_obj(param[1]);
    int done=0;
    int off = anna_buffer_get_count(buff);
    if(anna_entry_null(param[2]))
    {
	
	while(1)
	{
	    if(anna_buffer_get_capacity(buff) - off < READ_SZ)
	    {
		anna_buffer_set_capacity(buff, maxi(8192, 2*anna_buffer_get_capacity(buff)));
	    }
	    unsigned char *ptr = anna_buffer_get_payload(buff);
	    ssize_t rd = read(fd, &ptr[off], READ_SZ);
	    if(rd == -1)
	    {
		switch(errno)
		{
		    case EAGAIN:
			break;
			
		    default:
			return null_entry;
		}
	    }
	    else if(rd == 0)
	    {
		break;
	    }
	    off += rd;
	    done += rd;    
	    anna_buffer_set_count(buff, off);
	}
    }
    else
    {
	int count = anna_as_int(param[2]);
	
	anna_buffer_set_capacity(buff, count);
	unsigned char *ptr = anna_buffer_get_payload(buff);
	ssize_t rd = read(fd, &ptr[0], count);
	if(rd == -1)
	{
	    return null_entry;
	}
	done = rd;
    }
    
    return anna_from_int(done);
}

ANNA_VM_NATIVE(anna_cio_write, 3)
{
    if(anna_entry_null(param[0]) || anna_entry_null(param[1]))
    {
	return null_entry;
    }
    
    int fd = anna_as_int(param[0]);
    anna_object_t *buff = anna_as_obj(param[1]);
    int count = anna_buffer_get_count(buff);
    
    if(!anna_entry_null(param[2]))
    {
	count = maxi(0, mini(anna_as_int(param[2]), count));
    }
    
    unsigned char *ptr = anna_buffer_get_payload(buff);
    int res = write(fd, ptr, count);
    
    return (res != -1) ? anna_from_int(res) : null_entry;
}

ANNA_VM_NATIVE(anna_cio_close, 1)
{
    if(!anna_entry_null(param[0]))
    {
    	close(anna_as_int(param[0]));
    }
    return null_entry;
}

static void handle_stat(struct stat *buf, anna_object_t *list)
{
    anna_list_add(list, anna_from_int(buf->st_dev));
    anna_list_add(list, anna_from_int(buf->st_ino));
    anna_list_add(list, anna_from_int(buf->st_mode));
    anna_list_add(list, anna_from_int(buf->st_nlink));
    anna_list_add(list, anna_from_int(buf->st_uid));
    anna_list_add(list, anna_from_int(buf->st_gid));
    anna_list_add(list, anna_from_int(buf->st_rdev));
    anna_list_add(list, anna_from_int(buf->st_size));
    anna_list_add(list, anna_from_int(buf->st_blksize));
    anna_list_add(list, anna_from_int(buf->st_blocks));
    anna_list_add(list, anna_from_int(buf->st_atime));
    anna_list_add(list, anna_from_int(buf->st_mtime));
    anna_list_add(list, anna_from_int(buf->st_ctime));
}

ANNA_VM_NATIVE(anna_cio_stat, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    
    wchar_t *nam = anna_string_payload(anna_as_obj(param[0]));
    struct stat buf;
    if(wstat(nam, &buf))
    {
	free(nam);
	return null_entry;
    }
    free(nam);
    anna_object_t *res = anna_list_create_imutable(int_type);
    handle_stat(&buf, res);
    return anna_from_obj(res);
}

ANNA_VM_NATIVE(anna_cio_lstat, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    
    wchar_t *nam = anna_string_payload(anna_as_obj(param[0]));
    struct stat buf;
    if(lwstat(nam, &buf))
    {
	free(nam);
	return null_entry;
    }
    free(nam);
    anna_object_t *res = anna_list_create_imutable(int_type);
    handle_stat(&buf, res);
    return anna_from_obj(res);
}

ANNA_VM_NATIVE(anna_cio_fstat, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    
    int fd = anna_as_int(param[0]);
    struct stat buf;
    if(fstat(fd, &buf))
    {
	return null_entry;
    }
    anna_object_t *res = anna_list_create_imutable(int_type);
    handle_stat(&buf, res);
    return anna_from_obj(res);
}

ANNA_VM_NATIVE(anna_cio_mkdir, 2)
{
    if(anna_entry_null(param[0]) || anna_entry_null(param[1]))
    {
	return null_entry;
    }
    
    wchar_t *nam = anna_string_payload(
	anna_as_obj(param[0]));
    int mode = anna_as_int(param[1]);
    int res = wmkdir(nam, mode);
    if(res == -1)
    {
	return null_entry;
    }
    return anna_from_int(res);
}

ANNA_VM_NATIVE(anna_cio_unlink, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    
    wchar_t *nam = anna_string_payload(
	anna_as_obj(param[0]));
    int res = wunlink(nam);
    if(res == -1)
    {
	return null_entry;
    }
    return anna_from_int(res);
}

ANNA_VM_NATIVE(anna_cio_get_cwd, 1)
{
#define SZ 4096				\
    
    wchar_t buff[SZ];
    
    wchar_t *cwd = wgetcwd(buff, SZ);
    if(cwd)
    {
	return anna_from_obj(anna_string_create(wcslen(cwd), cwd));
    }
    
    return null_entry;
}

ANNA_VM_NATIVE(anna_cio_set_cwd, 2)
{
    if(anna_entry_null(param[1]))
    {
	return null_entry;
    }

    anna_entry_t *res = null_entry;
    
    wchar_t *dir = anna_string_payload(anna_as_obj(param[1]));
    if(wcslen(dir) != anna_string_get_count(anna_as_obj(param[1])))
    {	
	goto CLEANUP;
    }
    if(wchdir(dir))
    {
	goto CLEANUP;
    }
    
    res = param[1];
 
  CLEANUP:
    free(dir);
    return res;
}

ANNA_VM_NATIVE(anna_cio_is_relative, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    
    wchar_t *nam = anna_string_payload(
	anna_as_obj(param[0]));
    return (nam[0] != L'/') ? anna_from_int(1) : null_entry;
}

static void anna_open_mode_load(anna_stack_template_t *stack)
{
    anna_module_const_int(stack, L"readOnly", O_RDONLY, L"Open file in read-only mode.");
    anna_module_const_int(stack, L"writeOnly", O_WRONLY, L"Open file in write-only mode.");
    anna_module_const_int(stack, L"readWrite", O_RDWR, L"Open file in read-write mode.");

    anna_module_const_int(stack, L"append", O_APPEND, L"The file is opened in append mode.");
    anna_module_const_int(stack, L"async", O_ASYNC, L"Enable signal-driven I/O.");
    anna_module_const_int(stack, L"create", O_CREAT, L"If the file does not exist it will be created.");
    anna_module_const_int(stack, L"closeOnExec", O_CLOEXEC, L"Enable the close-on-exec flag for the new file descriptor.");
    anna_module_const_int(stack, L"direct", O_DIRECT, L"Try to minimize cache effects of the I/O to and from this file.");
    anna_module_const_int(stack, L"directory", O_DIRECTORY, L"If pathname is not a directory, cause the open to fail.");
    anna_module_const_int(stack, L"exclusive", O_EXCL, L"Ensure that this call creates the file: if this flag is specified in conjunction with O_CREAT, and pathname already exists, then open() will fail.");
    anna_module_const_int(stack, L"largeFile", O_LARGEFILE, L"(LFS) Allow files whose sizes cannot be represented in an off_t (but can be represented in an off64_t) to be opened.");
    anna_module_const_int(stack, L"noAccessTime", O_NOATIME, L"Do not update the file last access time (st_atime in the inode) when the file is read().");
    anna_module_const_int(stack, L"noControllingTTY", O_NOCTTY, L"If pathname refers to a terminal device - it will not become the process's controlling terminal even if the process does not have one.");
    anna_module_const_int(stack, L"noFollow", O_NOFOLLOW, L"If pathname is a symbolic link, then the open fails.");
    anna_module_const_int(stack, L"nonBlock", O_NONBLOCK, L"When possible, the file is opened in nonblocking mode.");
    anna_module_const_int(stack, L"synchronous", O_SYNC, L"The file is opened for synchronous I/O.");
    anna_module_const_int(stack, L"truncate", O_TRUNC, L"f the file already exists and is a regular file and the open mode allows writing (i.e., is writeOnly or readWrite) it will be truncated to length 0.");
}

static void anna_stat_mode_load(anna_stack_template_t *stack)
{
    anna_module_const_int(stack, L"regular", S_IFREG, 0);
    anna_module_const_int(stack, L"socket", S_IFSOCK, 0);
    anna_module_const_int(stack, L"link", S_IFLNK, 0);
    anna_module_const_int(stack, L"block", S_IFBLK, 0);
    anna_module_const_int(stack, L"directory", S_IFDIR, 0);
    anna_module_const_int(stack, L"character", S_IFCHR, 0);
    anna_module_const_int(stack, L"fifo", S_IFIFO, 0);
    
    anna_module_const_int(stack, L"suid", S_ISUID, 0);
    anna_module_const_int(stack, L"sgid", S_ISGID, 0);
    anna_module_const_int(stack, L"sticky", S_ISVTX, 0);
    
    anna_module_const_int(stack, L"userAll", S_IRWXU, 0);
    anna_module_const_int(stack, L"userRead", S_IRUSR, 0);
    anna_module_const_int(stack, L"userwrite", S_IWUSR, 0);
    anna_module_const_int(stack, L"userExecute", S_IXUSR, 0);
    
    anna_module_const_int(stack, L"groupAll", S_IRWXG, 0);
    anna_module_const_int(stack, L"groupRead", S_IRGRP, 0);
    anna_module_const_int(stack, L"groupwrite", S_IWGRP, 0);
    anna_module_const_int(stack, L"groupExecute", S_IXGRP, 0);
    
    anna_module_const_int(stack, L"otherAll", S_IRWXO, 0);
    anna_module_const_int(stack, L"otherRead", S_IROTH, 0);
    anna_module_const_int(stack, L"otherwrite", S_IWOTH, 0);
    anna_module_const_int(stack, L"otherExecute", S_IXOTH, 0);
}

void anna_cio_load(anna_stack_template_t *stack)
{
    anna_module_data_t modules[] = 
	{
	    { L"statMode", 0, anna_stat_mode_load },
	    { L"openMode", 0, anna_open_mode_load },
	};

    anna_module_data_create(modules, stack);

    wchar_t *o_argn[]={L"name", L"flags", L"mode"};
    anna_type_t *o_argv[] = {string_type, int_type, int_type};
	    
    anna_module_function(
	stack,
	L"open", 
	0, &anna_cio_open, 
	int_type, 
	3, o_argv, o_argn, 
	L"Open a file descriptor. Equivalent to the C open function. Returns null on failiure.");
    
    wchar_t *r_argn[]={L"fd", L"buffer", L"count"};
    anna_type_t *r_argv[] = {int_type, buffer_type, int_type};
    
    anna_module_function(
	stack,
	L"read", 
	0, &anna_cio_read, 
	int_type, 
	3, r_argv, r_argn, 
	L"Read from the specified file descriptor. \n\nWhen called with a specified file size, this function is equivalent to the C read function.\n\nWhen no size is given, the function will perform reads of 4096 bytes at a time until the file is empty or an error occurs. In this mode, the function will also retry reading when encountering the EAGAIN error. This usually happens when a signal was delivered to the process. Returns null on failure.");
    anna_module_function(
	stack,
	L"write", 
	0, &anna_cio_write, 
	int_type, 
	3, r_argv, r_argn, 
	L"Write to the specified file descriptor. Equivalent to the C write function. If size is null, the entire buffer will be written. Returns null on failure.");    
    wchar_t *c_argn[]={L"fd"};
    anna_type_t *c_argv[] = {int_type};
	    
    anna_module_function(
	stack, L"close", 
	0, &anna_cio_close, 
	object_type, 
	1, c_argv, c_argn, 
	L"Close the specified file descriptor. Equivalent to the C close function. Returns null on failiure.");

    anna_module_function(
	stack, L"stat", 
	0, &anna_cio_stat, 
	anna_list_type_get_imutable(int_type), 
	1, o_argv, o_argn, 
	L"Obtain status information on the file with the specified name. Equivalent to the C stat function.\n\nReturns the fields st_dev, st_ino, st_mode, st_nlink, st_uid, st_gid, st_rdev, st_size, st_blksize, st_blocks, st_atime, st_mtime and st_ctime as a list of integers. Returns null on failure.");

    anna_module_function(
	stack, L"lstat", 
	0, &anna_cio_lstat, 
	anna_list_type_get_imutable(int_type), 
	1, o_argv, o_argn, 
	L"Obtain status information on the file with the specified name. Does not follow symlinks. Equivalent to the C lstat function.\n\nReturns the fields st_dev, st_ino, st_mode, st_nlink, st_uid, st_gid, st_rdev, st_size, st_blksize, st_blocks, st_atime, st_mtime and st_ctime as a list of integers. Returns null on failure.");
    
    anna_module_function(
	stack, L"fstat", 
	0, &anna_cio_fstat, 
	anna_list_type_get_imutable(int_type), 
	1, c_argv, c_argn, 
	L"Obtain status information on the file connected to the specified file descriptor. Equivalent to the C fstat function.\n\nReturns the fields st_dev, st_ino, st_mode, st_nlink, st_uid, st_gid, st_rdev, st_size, st_blksize, st_blocks, st_atime, st_mtime and st_ctime as a list of integers. Returns null on failure.");

    anna_type_t *m_argv[] = 
	{
	    string_type, int_type
	}
    ;
    wchar_t *m_argn[] =
	{
	    L"pathname", L"mode"
	}
    ;
    anna_module_function(
	stack, L"mkdir", 
	0, &anna_cio_mkdir, 
	object_type,
	2, m_argv, m_argn, 
	L"Creates the specified directory. Equivalent to the C mkdir function. Returns null on failure.");

    anna_module_function(
	stack, L"unlink", 
	0, &anna_cio_unlink, 
	object_type,
	1, m_argv, m_argn, 
	L"Deletes a name from the file system. Equivalant to the C unlink function. Returns null on failure.");
    
    anna_module_const_int(stack, L"standardInput", 0, L"The default file descriptor for input");
    anna_module_const_int(stack, L"standardOutput", 1, L"The default file descriptor for output");
    anna_module_const_int(stack, L"standardError", 2, L"The default file descriptor for error messages");
    
    anna_module_const_char(stack, L"separator", L'/', L"The directory separator");

    anna_type_t *type = anna_stack_wrap(stack)->type;
    
    anna_member_create_native_property(
	type, anna_mid_get(L"cwd"),
	imutable_string_type,
	&anna_cio_get_cwd,
	&anna_cio_set_cwd,
	L"The current working directory."); 

    anna_module_function(
	stack, L"isRelative", 
	0, &anna_cio_is_relative, 
	object_type,
	1, m_argv, m_argn, 
	L"Checks if the specified file path is relative.");
    
}
