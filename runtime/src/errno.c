#include <errno.h>
#include <stdio.h>
#include <string.h>

#define FS_OK 0
#define FS_ERR_NO_DRV -1
#define FS_ERR_IO -2
#define FS_ERR_EXISTS -3
#define FS_ERR_NO_SPACE -4
#define FS_ERR_NAME_LONG -5
#define FS_ERR_NOT_INIT -6
#define FS_ERR_NO_NAME -7
#define FS_ERR_NO_TEXT -8
#define FS_ERR_NOT_EXISTS -9

static int _errno_var = 0;
int *__errno_location(void) { return &_errno_var; }

static const char *const __sys_errlist[] = {
    "Success",
    "Operation not permitted",
    "No such file or directory",
    "Input/output error",
    "Argument list too long",
    "Exec format error",
    "Resource temporarily unavailable",
    "Out of memory",
    "Permission denied",
    "Bad address",
    "Device or resource busy",
    "File exists",
    "No such device"
    "Not a directory",
    "Is a directory",
    "Invalid argument",
    "File table overflow",
    "Too many open files",
    "Text file busy",
    "File too large",
    "No space left on device",
    "Illegal seek",
    "Read-only file system",
    "Numerical argument out of domain",
    "Numerical result out of range",
    "Resource deadlock avoided",
    "File name too long",
    "No record locks available",
    "Function not implemented",
    "Directory not empty",
    "Too many symbolic links encountered",
    "No data available",
    "Timer expired",
    "Illegal byte sequence",
    "Quota exceeded",
};

int map_fs_error(int fs_error)
{
    switch (fs_error)
    {
    case FS_OK:
        return 0;
    case FS_ERR_NO_DRV:
        errno = ENODEV;
        return -1;
    case FS_ERR_IO:
        errno = EIO;
        return -1;
    case FS_ERR_EXISTS:
        errno = EEXIST;
        return -1;
    case FS_ERR_NO_SPACE:
        errno = ENOSPC;
        return -1;
    case FS_ERR_NAME_LONG:
        errno = ENAMETOOLONG;
        return -1;
    case FS_ERR_NOT_INIT:
        errno = ENOSYS;
        return -1;
    case FS_ERR_NO_NAME:
        errno = EINVAL;
        return -1;
    case FS_ERR_NOT_EXISTS:
        errno = ENOENT;
        return -1;
    default:
        errno = EIO;
        return -1;
    }
}

static const int __sys_nerr = (int)(sizeof(__sys_errlist) / sizeof(__sys_errlist[0]));

char *strerror(int errnum)
{
    if (errnum >= 0 && errnum < __sys_nerr)
        return (char *)__sys_errlist[errnum];
    return "Unknown error";
}

void perror(const char *s)
{
    const char *msg = strerror(errno);
    if (s && s[0] != '\0')
        fprintf(stderr, "%s: %s\n", s, msg);
    else
        fprintf(stderr, "%s\n", msg);
}
