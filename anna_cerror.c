#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common.h"
#include "util.h"
#include "wutil.h"
#include "anna.h"
#include "anna_module.h"
#include "anna_vm.h"
#include "anna_string.h"
#include "anna_cerror.h"
#include "anna_member.h"
#include "anna_mid.h"

ANNA_NATIVE(anna_cerror_strerror, 1)
{
    if(anna_entry_null(param[0]))
    {
	return null_entry;
    }
    int err = anna_as_int(param[0]);
    
    char *desc = strerror(err);
    wchar_t *wdesc = str2wcs(desc);
    anna_object_t *res = anna_string_create(wcslen(wdesc), wdesc);
    free(wdesc);
    return anna_from_obj(res);
}

ANNA_NATIVE(anna_cerror_get_errno, 1)
{
    return anna_from_int(errno);    
}

void anna_cerror_load(anna_stack_template_t *stack)
{
    wchar_t *s_argn[]={L"error"};
    anna_type_t *s_argv[] = {int_type};
    
    anna_module_function(
	stack, L"strError", 
	0, &anna_cerror_strerror, 
	string_type, 
	1, s_argv, s_argn, 
	L"Returns a string that describes the error code passed in the argument.");
    
    anna_module_const_int(stack, L"tooBig", E2BIG, 0);
    anna_module_const_int(stack, L"access", EACCES, 0);
    anna_module_const_int(stack, L"addressInUse", EADDRINUSE, 0);
    anna_module_const_int(stack, L"addressNotAvailable", EADDRNOTAVAIL, 0);
    anna_module_const_int(stack, L"addressFamilyNotSupported", EAFNOSUPPORT, 0);
    anna_module_const_int(stack, L"again", EAGAIN, 0);
    anna_module_const_int(stack, L"already", EALREADY, 0);
    anna_module_const_int(stack, L"badExchange", EBADE, 0);
    anna_module_const_int(stack, L"badFileDescriptor", EBADF, 0);
    anna_module_const_int(stack, L"badFileDescriptorState", EBADFD, 0);
    
    anna_module_const_int(stack, L"exist", EEXIST, 0);
    anna_module_const_int(stack, L"fault", EFAULT, 0);
    anna_module_const_int(stack, L"loop", ELOOP, 0);
    anna_module_const_int(stack, L"maxLink", EMLINK, 0);
    anna_module_const_int(stack, L"nameTooLong", ENAMETOOLONG, 0);
    anna_module_const_int(stack, L"noEntry", ENOENT, 0);
    anna_module_const_int(stack, L"noMemory", ENOMEM, 0);
    anna_module_const_int(stack, L"noSpace", ENOSPC, 0);
    anna_module_const_int(stack, L"notDirectory", ENOTDIR, 0);
    anna_module_const_int(stack, L"permission", EPERM, 0);
    anna_module_const_int(stack, L"readOnly", EROFS, 0);
    
    anna_type_t *type = anna_stack_wrap(stack)->type;
    anna_member_create_native_property(
	type, anna_mid_get(L"errno"),
	int_type,
	&anna_cerror_get_errno,
	0);
}

