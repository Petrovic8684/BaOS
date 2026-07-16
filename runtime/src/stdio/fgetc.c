#include <stdio.h>
#include "internal/syscalls.h"
#include <errno.h>
#include "stdio/file_internal.h"

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

    if (stream->mode == 0)
    {
        unsigned int file_size = file_read_size(stream);
        if (stream->pos >= file_size)
        {
            stream->eof = 1;
            errno = EAGAIN;
            return EOF;
        }

        unsigned int chunk_off = file_read_chunk_off(stream);
        if (stream->pos < chunk_off || stream->pos >= chunk_off + stream->buf_end)
        {
            if (file_refill_read(stream) != 0)
            {
                stream->err = 1;
                return EOF;
            }
            chunk_off = file_read_chunk_off(stream);
        }

        if (stream->buf_end == 0)
        {
            stream->eof = 1;
            errno = EAGAIN;
            return EOF;
        }

        unsigned int rel = stream->pos - chunk_off;
        int ch = (unsigned char)stream->buf[rel];
        stream->pos++;
        if (stream->pos >= file_size)
            stream->eof = 1;

        return ch;
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
