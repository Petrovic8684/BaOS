#include <stdio.h>
#include "internal/syscalls.h"

int puts(const char *s)
{
    fputs(s, stdout);
    fputc('\n', stdout);
    return 0;
}
