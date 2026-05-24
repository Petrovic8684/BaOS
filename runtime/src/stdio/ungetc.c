#include <stdio.h>
#include "internal/syscalls.h"
#include <errno.h>
#include "stdio/file_internal.h"

int ungetc(int c, FILE *stream)
{
    if (stream == stdin)
    {
        stdin_ungetc = c & 0xFF;
        return c;
    }
    if (!stream || stream->buf_pos == 0)
    {
        errno = EINVAL;
        return EOF;
    }

    stream->buf[--stream->buf_pos] = (unsigned char)c;
    return c;
}
