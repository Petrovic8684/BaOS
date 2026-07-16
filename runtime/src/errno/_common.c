#include <errno.h>

static int _errno_var = 0;

int *__errno_location(void)
{
    return &_errno_var;
}

const char *const __sys_errlist[] = {
    [0] = "Success",
    [EPERM] = "Operation not permitted",
    [ENOENT] = "No such file or directory",
    [EIO] = "Input/output error",
    [E2BIG] = "Argument list too long",
    [ENOEXEC] = "Exec format error",
    [EAGAIN] = "Resource temporarily unavailable",
    [ENOMEM] = "Out of memory",
    [EACCES] = "Permission denied",
    [EFAULT] = "Bad address",
    [EBUSY] = "Device or resource busy",
    [EEXIST] = "File exists",
    [ENODEV] = "No such device",
    [ENOTDIR] = "Not a directory",
    [EISDIR] = "Is a directory",
    [EINVAL] = "Invalid argument",
    [ENFILE] = "File table overflow",
    [EMFILE] = "Too many open files",
    [ETXTBSY] = "Text file busy",
    [EFBIG] = "File too large",
    [ENOSPC] = "No space left on device",
    [ESPIPE] = "Illegal seek",
    [EROFS] = "Read-only file system",
    [EDOM] = "Numerical argument out of domain",
    [ERANGE] = "Numerical result out of range",
    [EDEADLK] = "Resource deadlock avoided",
    [ENAMETOOLONG] = "File name too long",
    [ENOSYS] = "Function not implemented",
    [ENOTEMPTY] = "Directory not empty",
    [ELOOP] = "Too many symbolic links encountered",
    [ENODATA] = "No data available",
    [ETIME] = "Timer expired",
    [EILSEQ] = "Illegal byte sequence",
    [EDQUOT] = "Quota exceeded",
};

const int __sys_nerr = (int)(sizeof(__sys_errlist) / sizeof(__sys_errlist[0]));
