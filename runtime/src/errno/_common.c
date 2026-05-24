#include <errno.h>

static int _errno_var = 0;


int *__errno_location(void) { return &_errno_var; }

const char *const __sys_errlist[] = {
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

const int __sys_nerr = (int)(sizeof(__sys_errlist) / sizeof(__sys_errlist[0]));
