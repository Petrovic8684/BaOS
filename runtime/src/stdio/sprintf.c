#include <stdio.h>
#include "internal/syscalls.h"
#include <stdarg.h>

int sprintf(char *str, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int r = vsnprintf(str, (size_t)-1, format, ap);
    va_end(ap);
    return r;
}
