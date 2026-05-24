#include <stdio.h>
#include "internal/syscalls.h"
#include <errno.h>

void rewind(FILE *stream)
{
    if (!stream)
    {
        errno = EINVAL;
        return;
    }
    fseek(stream, 0, SEEK_SET);
    clearerr(stream);
}
