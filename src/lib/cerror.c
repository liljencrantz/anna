#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "anna/common.h"
#include "anna/util.h"
#include "anna/wutil.h"
#include "anna/base.h"
#include "anna/module.h"
#include "anna/vm.h"
#include "anna/member.h"
#include "anna/mid.h"

#include "anna/lib/lang/string.h"
#include "anna/lib/clib.h"

ANNA_VM_NATIVE(anna_cerror_strerror, 1)
{
    int err = errno;
    
    if(!anna_entry_null(param[0]))
    {
	err = anna_as_int(param[0]);
    }
    
    char *desc = strerror(err);
    if(!desc)
    {
	return null_entry;
    }
    
    wchar_t *wdesc = str2wcs(desc);
    anna_object_t *res = anna_string_create(wcslen(wdesc), wdesc);
    free(wdesc);
    return anna_from_obj(res);
}

ANNA_VM_NATIVE(anna_cerror_get_errno, 1)
{
    return anna_from_int(errno);    
}

void anna_cerror_load(anna_stack_template_t *stack)
{
    wchar_t *s_argn[]={L"error"};
    anna_type_t *s_argv[] = {int_type};
    
    anna_module_function(
	stack, L"errorString", 
	0, &anna_cerror_strerror, 
	string_type, 
	1, s_argv, s_argn, 
	L"Returns a string that describes the error code passed in the argument. Equivalent to the C strerror function.");
    
    anna_module_const_int(stack, L"tooBig", E2BIG, L"Argument list too long.");
    anna_module_const_int(stack, L"access", EACCES, L"Permission denied.");
    anna_module_const_int(stack, L"addressInUse", EADDRINUSE, L"Address already in use.");
    anna_module_const_int(stack, L"addressNotAvailable", EADDRNOTAVAIL, L"Address not available.");
    anna_module_const_int(stack, L"addressFamilyNotSupported", EAFNOSUPPORT, L"Address family not supported.");
    anna_module_const_int(stack, L"again", EAGAIN, L"Resource temporarily unavailable (may be the same value as wouldBlock).");
    anna_module_const_int(stack, L"alreadyInProgress", EALREADY, L"Connection already in progress.");
    anna_module_const_int(stack, L"badExchange", EBADE, L"Invalid exchange.");
    anna_module_const_int(stack, L"badFileDescriptor", EBADF, L"Bad file descriptor.");
    anna_module_const_int(stack, L"badFileDescriptorState", EBADFD, L"File descriptor in bad state.");    
    anna_module_const_int(stack, L"badRequest", EBADR, L"Invalid request descriptor.");
    anna_module_const_int(stack, L"badRequestCode", EBADRQC, L"Invalid request code.");
    anna_module_const_int(stack, L"badSlot", EBADSLT, L"Invalid slot.");
    anna_module_const_int(stack, L"deviceBusy", EBUSY, L"Device or resource busy.");
    anna_module_const_int(stack, L"canceled", ECANCELED, L"Operation canceled.");
    anna_module_const_int(stack, L"noChild", ECHILD, L"No child processes.");
    anna_module_const_int(stack, L"channelRange", ECHRNG, L"Channel number out of range.");
    anna_module_const_int(stack, L"communicationError", ECOMM, L"Communication error on send.");
    anna_module_const_int(stack, L"connectionAborted", ECONNABORTED, L"Connection aborted.");
    anna_module_const_int(stack, L"connectionRefused", ECONNREFUSED, L"Connection refused.");
    anna_module_const_int(stack, L"connectionReset", ECONNRESET, L"Connection reset.");
    anna_module_const_int(stack, L"deadlock", EDEADLK, L"Resource deadlock avoided.");
    anna_module_const_int(stack, L"destinationRequired", EDESTADDRREQ, L"Destination address required.");
    anna_module_const_int(stack, L"domain", EDOM, L"Mathematics argument out of domain of function.");
    anna_module_const_int(stack, L"diskQuota", EDQUOT, L"Disk quota exceeded.");
    anna_module_const_int(stack, L"exist", EEXIST, L"File exists.");
    anna_module_const_int(stack, L"fault", EFAULT, L"Bad address.");
    anna_module_const_int(stack, L"fileTooBig", EFBIG, L"File too large.");
    anna_module_const_int(stack, L"hostDown", EHOSTDOWN, L"Host is down.");
    anna_module_const_int(stack, L"hostUnreachable", EHOSTUNREACH, L"Host is unreachable.");
    anna_module_const_int(stack, L"identifierRemoved", EIDRM, L"Identifier removed.");
    anna_module_const_int(stack, L"illegalByteSequence", EILSEQ, L"Illegal byte sequence.");
    anna_module_const_int(stack, L"inProgress", EINPROGRESS, L"Operation in progress.");
    anna_module_const_int(stack, L"interrupted", EINTR, L"Interrupted function call.");
    anna_module_const_int(stack, L"invalid", EINVAL, L"Invalid argument.");
    anna_module_const_int(stack, L"ioError", EIO, L"Input/output error.");
    anna_module_const_int(stack, L"isConencted", EISCONN, L"Socket is connected.");
    anna_module_const_int(stack, L"isDirectory", EISDIR, L"Is a directory.");
    anna_module_const_int(stack, L"isNamed", EISNAM, L"Is a named type file.");
    anna_module_const_int(stack, L"keyExpired", EKEYEXPIRED, L"Key has expired.");
    anna_module_const_int(stack, L"keyRejected", EKEYREJECTED, L"Key was rejected by service.");
    anna_module_const_int(stack, L"keyRevoked", EKEYREVOKED, L"Key has been revoked.");
    anna_module_const_int(stack, L"level2Halted", EL2HLT, L"Level 2 halted.");
    anna_module_const_int(stack, L"level2NotSynced", EL2NSYNC, L"Level 2 not synchronized.");
    anna_module_const_int(stack, L"level3Halted", EL3HLT, L"Level 3 halted.");
    anna_module_const_int(stack, L"level3Reset", EL3RST, L"Level 3 reset.");
    anna_module_const_int(stack, L"libraryAccess", ELIBACC, L"Cannot access a needed shared library.");
    anna_module_const_int(stack, L"libraryBad", ELIBBAD, L"Accessing a corrupted shared library.");
    anna_module_const_int(stack, L"libraryMax", ELIBMAX, L"Attempting to link in too many shared libraries.");
    anna_module_const_int(stack, L"libraryCorrupt", ELIBSCN, L"lib section in a.out corrupted.");
    anna_module_const_int(stack, L"libraryExec", ELIBEXEC, L"Cannot exec a shared library directly.");
    anna_module_const_int(stack, L"loop", ELOOP, L"Too many levels of symbolic links");
    anna_module_const_int(stack, L"mediumType", EMEDIUMTYPE, L"Wrong medium type.");
    anna_module_const_int(stack, L"maxFile", EMFILE, L"Too many open files.");
    anna_module_const_int(stack, L"maxLink", EMLINK, L"Too many links.");
    anna_module_const_int(stack, L"messageSize", EMSGSIZE, L"Message too long.");
    anna_module_const_int(stack, L"multihop", EMULTIHOP, L"Multihop attempted.");
    anna_module_const_int(stack, L"nameTooLong", ENAMETOOLONG, L"Filename too long.");
    anna_module_const_int(stack, L"netDown", ENETDOWN, L"Network is down.");
    anna_module_const_int(stack, L"netReset", ENETRESET, L"Connection aborted by network.");
    anna_module_const_int(stack, L"netUnreachable", ENETUNREACH, L"Network unreachable.");
    anna_module_const_int(stack, L"maxFileInSystem", ENFILE, L"Too many open files in system.");
    anna_module_const_int(stack, L"noBufferSpace", ENOBUFS, L"No buffer space available.");
    anna_module_const_int(stack, L"noData", ENODATA, L"No message is available on the STREAM head read queue.");
    anna_module_const_int(stack, L"noDevice", ENODEV, L"No such device.");
    anna_module_const_int(stack, L"noEntry", ENOENT, L"No such file or directory.");
    anna_module_const_int(stack, L"noExec", ENOEXEC, L"Exec format error.");
    anna_module_const_int(stack, L"noKey", ENOKEY, L"Required key not available.");
    anna_module_const_int(stack, L"noLock", ENOLCK, L"No locks available.");
    anna_module_const_int(stack, L"noLink", ENOLINK, L"Link has been severed.");
    anna_module_const_int(stack, L"noMedium", ENOMEDIUM, L"No medium found.");
    anna_module_const_int(stack, L"noMemory", ENOMEM, L"Not enough space.");
    anna_module_const_int(stack, L"noMessage", ENOMSG, L"No message of the desired type.");
    anna_module_const_int(stack, L"noNet", ENONET, L"Machine is not on the network.");
    anna_module_const_int(stack, L"noPackage", ENOPKG, L"Package not installed.");
    anna_module_const_int(stack, L"noProtocol", ENOPROTOOPT, L"Protocol not available.");
    anna_module_const_int(stack, L"noSpace", ENOSPC, L"No space left on device.");
    anna_module_const_int(stack, L"noStream", ENOSR, L"No STREAM resources.");
    anna_module_const_int(stack, L"notStream", ENOSTR, L"Not a STREAM.");
    anna_module_const_int(stack, L"notImplemented", ENOSYS, L"Function not implemented.");
    anna_module_const_int(stack, L"notBlock", ENOTBLK, L"Block device required.");
    anna_module_const_int(stack, L"notConnected", ENOTCONN, L"The socket is not connected.");
    anna_module_const_int(stack, L"notDirectory", ENOTDIR, L"Not a directory.");
    anna_module_const_int(stack, L"notEmpty", ENOTEMPTY, L"Directory not empty.");
    anna_module_const_int(stack, L"notSocket", ENOTSOCK, L"Not a socket.");
    anna_module_const_int(stack, L"notSupported", ENOTSUP, L"Operation not supported.");
    anna_module_const_int(stack, L"notTty", ENOTTY, L"Inappropriate I/O control operation.");
    anna_module_const_int(stack, L"notUnique", ENOTUNIQ, L"Name not unique on network.");
    anna_module_const_int(stack, L"noDeviceOrAddress", ENXIO, L"No such device or address.");
    anna_module_const_int(stack, L"operationNotSupported", EOPNOTSUPP, L"Operation not supported on socket.");
    anna_module_const_int(stack, L"overflow", EOVERFLOW, L"Value too large to be stored in data type.");
    anna_module_const_int(stack, L"permission", EPERM, L"Operation not permitted.");
    anna_module_const_int(stack, L"protocolFamilyNotSupported", EPFNOSUPPORT, L"Protocol family not supported.");
    anna_module_const_int(stack, L"pipe", EPIPE, L"Broken pipe.");
    anna_module_const_int(stack, L"protocolError", EPROTO, L"Protocol error.");
    anna_module_const_int(stack, L"protocolNotSupported", EPROTONOSUPPORT, L"Protocol not supported.");
    anna_module_const_int(stack, L"protocolType", EPROTOTYPE, L"Protocol wrong type for socket.");
    anna_module_const_int(stack, L"range", ERANGE, L"Result too large.");
    anna_module_const_int(stack, L"remoteAddressChanged", EREMCHG, L"Remote address changed.");
    anna_module_const_int(stack, L"remote", EREMOTE, L"Object is remote.");
    anna_module_const_int(stack, L"remoteIo", EREMOTEIO, L"Remote I/O error.");
    anna_module_const_int(stack, L"restart", ERESTART, L"Interrupted system call should be restarted.");
    anna_module_const_int(stack, L"readOnly", EROFS, L"Read-only file system.");
    anna_module_const_int(stack, L"shutdown", ESHUTDOWN, L"Cannot send after transport endpoint shutdown.");
    anna_module_const_int(stack, L"seekPipe", ESPIPE, L"Invalid seek.");
    anna_module_const_int(stack, L"socketNotSupported", ESOCKTNOSUPPORT, L"Socket type not supported."); 
    anna_module_const_int(stack, L"noSuchProcess", ESRCH, L"No such process.");
    anna_module_const_int(stack, L"stale", ESTALE, L"Stale file handle. This error can occur for NFS and for other file systems.");
    anna_module_const_int(stack, L"streamPipe", ESTRPIPE, L"Streams pipe error.");
    anna_module_const_int(stack, L"timer", ETIME, L"Timer expired.");
    anna_module_const_int(stack, L"timeout", ETIMEDOUT, L"Connection timed out.");
    anna_module_const_int(stack, L"textBusy", ETXTBSY, L"Text file busy.");
    anna_module_const_int(stack, L"unclean", EUCLEAN, L"Structure needs cleaning.");
    anna_module_const_int(stack, L"unattached", EUNATCH, L"Protocol driver not attached.");
    anna_module_const_int(stack, L"users", EUSERS, L"Too many users.");
    anna_module_const_int(stack, L"wouldBlock", EWOULDBLOCK, L"Operation would block (may be same value as again).");
    anna_module_const_int(stack, L"improperLink", EXDEV, L"Improper link.");
    anna_module_const_int(stack, L"exchangeFull", EXFULL, L"Exchange full.");

    anna_type_t *type = anna_stack_wrap(stack)->type;
    anna_member_create_native_property(
	type, anna_mid_get(L"errno"),
	int_type,
	&anna_cerror_get_errno,
	0,
	L"The latest C library error to occur");
}

