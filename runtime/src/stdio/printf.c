#include <stdio.h>
#include "internal/syscalls.h"
#include <stdarg.h>

int printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(stdout, fmt, args);
    va_end(args);
    return ret;
}
