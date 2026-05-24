#include <stdio.h>
#include "internal/syscalls.h"
#include <stdlib.h>
#include <errno.h>

int fputc(int c, FILE *stream)
{
    if (!stream)
    {
        errno = EINVAL;
        return EOF;
    }

    if (stream == stdout || stream == stderr)
    {
        char tmp[2] = {(char)c, 0};
        write(tmp);
        return c;
    }

    if (!stream->buf)
    {
        stream->buf = (unsigned char *)malloc(128);
        if (!stream->buf)
        {
            stream->err = 1;
            return EOF;
        }
        stream->buf_pos = 0;
        stream->buf_end = 128;
    }
    else if (stream->buf_pos >= stream->buf_end)
    {
        unsigned char *new_buf = (unsigned char *)realloc(stream->buf, stream->buf_end * 2);
        if (!new_buf)
        {
            stream->err = 1;
            return EOF;
        }
        stream->buf = new_buf;
        stream->buf_end *= 2;
    }

    stream->buf[stream->buf_pos++] = (unsigned char)c;
    return c;
}
