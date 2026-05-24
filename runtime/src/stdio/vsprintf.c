#include <stdio.h>
#include "internal/syscalls.h"
#include <stdarg.h>

int vsprintf(char *str, const char *format, va_list ap)
{
    return vsnprintf(str, (size_t)-1, format, ap);
}
