#include <stdio.h>
#include "internal/syscalls.h"
#include <errno.h>

int ferror(FILE *stream)
{
    if (!stream)
    {
        errno = EINVAL;
        return 0;
    }
    return stream->err;
}
