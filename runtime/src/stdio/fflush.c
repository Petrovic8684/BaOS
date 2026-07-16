#include <stdio.h>
#include "internal/syscalls.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "internal/fs_helpers.h"

int fflush(FILE *stream)
{
    if (!stream)
    {
        errno = EINVAL;
        return EOF;
    }

    if (stream == stdout || stream == stderr)
    {
        errno = ESPIPE;
        return 0;
    }

    if (stream->buf_pos == 0)
        return 0;

    int wres;

    if (stream->mode == 1)
    {
        wres = fs_write_file(stream->name, stream->buf, (unsigned int)stream->buf_pos);
    }
    else
    {
        /* Append mode keeps existing file bytes in stream->buf[0..buf_pos). */
        wres = fs_write_file(stream->name, stream->buf, (unsigned int)stream->buf_pos);
    }

    stream->buf_pos = 0;

    if (wres < 0)
    {
        stream->err = 1;
        return EOF;
    }

    return 0;
}
