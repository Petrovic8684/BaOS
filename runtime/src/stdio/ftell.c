#include <stdio.h>
#include "internal/syscalls.h"
#include <errno.h>

long ftell(FILE *stream)
{
    if (!stream)
    {
        errno = EINVAL;
        return -1;
    }
    return (long)stream->buf_pos;
}
