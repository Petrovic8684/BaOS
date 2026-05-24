#include <stdio.h>
#include "internal/syscalls.h"
#include <errno.h>

int fgetc(FILE *stream)
{
    if (!stream)
    {
        errno = EINVAL;
        return EOF;
    }

    if (stream == stdout || stream == stderr)
    {
        errno = ESPIPE;
        return EOF;
    }

    if (stream->buf_pos >= stream->buf_end)
    {
        stream->eof = 1;
        errno = EAGAIN;
        return EOF;
    }

    int ch = (unsigned char)stream->buf[stream->buf_pos++];
    if (stream->buf_pos >= stream->buf_end)
        stream->eof = 1;

    return ch;
}
