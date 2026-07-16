#include <stdio.h>
#include "internal/syscalls.h"
#include <errno.h>
#include "stdio/file_internal.h"

long ftell(FILE *stream)
{
    if (!stream)
    {
        errno = EINVAL;
        return -1;
    }

    if (stream->mode == 0)
        return (long)stream->pos;

    return (long)stream->buf_pos;
}
