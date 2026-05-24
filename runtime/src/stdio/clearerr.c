#include <stdio.h>
#include "internal/syscalls.h"
#include <errno.h>

void clearerr(FILE *stream)
{
    if (!stream)
    {
        errno = EINVAL;
        return;
    }
    stream->err = 0;
    stream->eof = 0;
}
