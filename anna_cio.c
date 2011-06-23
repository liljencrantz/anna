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

#include "common.h"
#include "util.h"
#include "wutil.h"
#include "anna.h"
#include "anna_module.h"
#include "anna_module_data.h"
#include "anna_cio.h"
#include "anna_stack.h"
#include "anna_vm.h"
#include "anna_string.h"
#include "anna_function.h"
#include "anna_buffer.h"
#include "anna_intern.h"
#include "anna_list.h"

#define READ_SZ 4096

ANNA_NATIVE(anna_cio_open, 3)
{
    if(anna_entry_null(param[0]) || anna_entry_null(param[1]) || anna_entry_null(param[1]))
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

ANNA_NATIVE(anna_cio_read, 3)
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

ANNA_NATIVE(anna_cio_write, 3)
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

ANNA_NATIVE(anna_cio_close, 1)
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

ANNA_NATIVE(anna_cio_stat, 1)
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

ANNA_NATIVE(anna_cio_lstat, 1)
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

ANNA_NATIVE(anna_cio_fstat, 1)
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

ANNA_NATIVE(anna_cio_mkdir, 2)
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

static void anna_open_mode_load(anna_stack_template_t *stack)
{
    anna_module_const_int(stack, L"readOnly", O_RDONLY);
    anna_module_const_int(stack, L"writeOnly", O_WRONLY);
    anna_module_const_int(stack, L"readWrite", O_RDWR);

    anna_module_const_int(stack, L"append", O_APPEND);
    anna_module_const_int(stack, L"create", O_CREAT);
    anna_module_const_int(stack, L"closeOnExec", O_CLOEXEC);
    anna_module_const_int(stack, L"direct", O_DIRECT);    
    anna_module_const_int(stack, L"directory", O_DIRECTORY);
    anna_module_const_int(stack, L"exclusive", O_EXCL);
    anna_module_const_int(stack, L"largeFile", O_LARGEFILE);
    anna_module_const_int(stack, L"noAccessTime", O_NOATIME);
    anna_module_const_int(stack, L"noControllingTTY", O_NOCTTY);
    anna_module_const_int(stack, L"noFollow", O_NOFOLLOW);
    anna_module_const_int(stack, L"nonBlock", O_NONBLOCK);
    anna_module_const_int(stack, L"synchronous", O_SYNC);
    anna_module_const_int(stack, L"truncate", O_TRUNC);
}

static void anna_stat_mode_load(anna_stack_template_t *stack)
{
    anna_module_const_int(stack, L"regular", S_IFREG);
    anna_module_const_int(stack, L"socket", S_IFSOCK);
    anna_module_const_int(stack, L"link", S_IFLNK);
    anna_module_const_int(stack, L"block", S_IFBLK);
    anna_module_const_int(stack, L"directory", S_IFDIR);
    anna_module_const_int(stack, L"character", S_IFCHR);
    anna_module_const_int(stack, L"fifo", S_IFIFO);
    
    anna_module_const_int(stack, L"suid", S_ISUID);
    anna_module_const_int(stack, L"sgid", S_ISGID);
    anna_module_const_int(stack, L"sticky", S_ISVTX);
    
    anna_module_const_int(stack, L"userAll", S_IRWXU);
    anna_module_const_int(stack, L"userRead", S_IRUSR);
    anna_module_const_int(stack, L"userwrite", S_IWUSR);
    anna_module_const_int(stack, L"userExecute", S_IXUSR);
    
    anna_module_const_int(stack, L"groupAll", S_IRWXG);
    anna_module_const_int(stack, L"groupRead", S_IRGRP);
    anna_module_const_int(stack, L"groupwrite", S_IWGRP);
    anna_module_const_int(stack, L"groupExecute", S_IXGRP);
    
    anna_module_const_int(stack, L"otherAll", S_IRWXO);
    anna_module_const_int(stack, L"otherRead", S_IROTH);
    anna_module_const_int(stack, L"otherwrite", S_IWOTH);
    anna_module_const_int(stack, L"otherExecute", S_IXOTH);
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
	L"Open a file descriptor. Equivalent to the C open function.");
    
    wchar_t *r_argn[]={L"fd", L"buffer", L"count"};
    anna_type_t *r_argv[] = {int_type, buffer_type, int_type};
    
    anna_module_function(
	stack,
	L"read", 
	0, &anna_cio_read, 
	int_type, 
	3, r_argv, r_argn, 
	L"Read from the specified file descriptor. \n\nWhen called with a specified file size, this function is equivalent to the C read function.\n\nWhen no size is given, the function will perform reads of 4096 bytes at a time until the file is empty or an error occurs. In this mode, the function will also retry reading when encountering the EAGAIN error. This usually happens when a signal was delivered to the process.");
    anna_module_function(
	stack,
	L"write", 
	0, &anna_cio_write, 
	int_type, 
	3, r_argv, r_argn, 
	L"Write to the specified file descriptor. Equivalent to the C write function. If size is null, the entire buffer will be written.");    
    wchar_t *c_argn[]={L"fd"};
    anna_type_t *c_argv[] = {int_type};
	    
    anna_module_function(
	stack, L"close", 
	0, &anna_cio_close, 
	object_type, 
	1, c_argv, c_argn, 
	L"Close the specified file descriptor. Equivalent to the C close function.");

    anna_module_function(
	stack, L"stat", 
	0, &anna_cio_stat, 
	anna_list_type_get_imutable(int_type), 
	1, o_argv, o_argn, 
	L"Obtain status information on the file with the specified name. Equivalent to the C stat function.\n\nReturns the fields st_dev, st_ino, st_mode, st_nlink, st_uid, st_gid, st_rdev, st_size, st_blksize, st_blocks, st_atime, st_mtime and st_ctime as a list of integers.");

    anna_module_function(
	stack, L"lstat", 
	0, &anna_cio_lstat, 
	anna_list_type_get_imutable(int_type), 
	1, o_argv, o_argn, 
	L"Obtain status information on the file with the specified name. Does not follow symlinks. Equivalent to the C lstat function.\n\nReturns the fields st_dev, st_ino, st_mode, st_nlink, st_uid, st_gid, st_rdev, st_size, st_blksize, st_blocks, st_atime, st_mtime and st_ctime as a list of integers.");
    
    anna_module_function(
	stack, L"fstat", 
	0, &anna_cio_fstat, 
	anna_list_type_get_imutable(int_type), 
	1, c_argv, c_argn, 
	L"Obtain status information on the file connected to the specified file descriptor. Equivalent to the C fstat function.\n\nReturns the fields st_dev, st_ino, st_mode, st_nlink, st_uid, st_gid, st_rdev, st_size, st_blksize, st_blocks, st_atime, st_mtime and st_ctime as a list of integers.");

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
	L"Creates the specified directory. Equivalent to the C mkdir function.");
    
    anna_module_const_int(stack, L"standardInput", 0);
    anna_module_const_int(stack, L"standardOutput", 1);
    anna_module_const_int(stack, L"standardError", 2);
}

