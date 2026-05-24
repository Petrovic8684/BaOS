#include <stdio.h>
#include "internal/syscalls.h"
#include "stdio/file_internal.h"

int getchar(void)
{
    if (stdin_ungetc != -1)
    {
        int c = stdin_ungetc;
        stdin_ungetc = -1;
        return c;
    }
    return (int)sys_read();
}
