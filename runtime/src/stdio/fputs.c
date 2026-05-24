#include <stdio.h>
#include "internal/syscalls.h"

int fputs(const char *s, FILE *stream)
{
    while (*s)
        fputc(*s++, stream);
    return 0;
}
