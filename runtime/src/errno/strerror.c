#include <errno.h>
#include "errno/errno_internal.h"



char *strerror(int errnum)
{
    if (errnum >= 0 && errnum < __sys_nerr && __sys_errlist[errnum])
        return (char *)__sys_errlist[errnum];
    return "Unknown error";
}
