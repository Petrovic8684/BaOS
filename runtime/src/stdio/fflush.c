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

    int file_size_i = fs_read_file_size(stream->name);
    if (file_size_i < 0)
        file_size_i = 0;

    size_t file_size = (size_t)file_size_i;
    size_t need = file_size + (size_t)stream->buf_pos;

    if (need < file_size || need < (size_t)stream->buf_pos)
    {
        stream->err = 1;
        errno = EFBIG;
        return EOF;
    }

    unsigned char *combined = (unsigned char *)malloc(need);
    if (!combined)
    {
        stream->err = 1;
        return EOF;
    }

    unsigned int got = 0;
    if (file_size > 0)
        if (fs_read_file(stream->name, combined, (unsigned int)file_size, &got) != 0)
            got = 0;

    memcpy(combined + got, stream->buf, (size_t)stream->buf_pos);

    int wres = fs_write_file(stream->name, combined, (unsigned int)(got + stream->buf_pos));
    free(combined);
    stream->buf_pos = 0;

    if (wres < 0)
    {
        stream->err = 1;
        return EOF;
    }

    return 0;
}
