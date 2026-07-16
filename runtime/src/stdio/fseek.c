#include <stdio.h>
#include "internal/syscalls.h"
#include "internal/fs_helpers.h"
#include <errno.h>
#include "stdio/file_internal.h"

int fseek(FILE *stream, long offset, int whence)
{
    if (!stream)
    {
        errno = EINVAL;
        return -1;
    }
    if (stream == stdout || stream == stderr)
    {
        errno = ESPIPE;
        return -1;
    }

    if (stream->mode == 0)
    {
        unsigned int file_size = file_read_size(stream);
        long target;

        if (whence == SEEK_SET)
            target = offset;
        else if (whence == SEEK_CUR)
            target = (long)stream->pos + offset;
        else if (whence == SEEK_END)
            target = (long)file_size + offset;
        else
        {
            errno = EINVAL;
            return -1;
        }

        if (target < 0 || (unsigned long)target > file_size)
        {
            errno = EINVAL;
            return -1;
        }

        stream->pos = (unsigned int)target;
        stream->eof = (stream->pos >= file_size);
        stream->err = 0;
        return 0;
    }

    if (whence == SEEK_SET)
    {
        if (offset < 0)
        {
            errno = EINVAL;
            return -1;
        }
        if ((unsigned long)offset <= stream->buf_end)
        {
            stream->buf_pos = (unsigned int)offset;
            stream->eof = (stream->buf_pos >= stream->buf_end);
            return 0;
        }
        errno = EINVAL;
        return -1;
    }
    else if (whence == SEEK_CUR)
    {
        long newpos = (long)stream->buf_pos + offset;
        return fseek(stream, newpos, SEEK_SET);
    }
    else if (whence == SEEK_END)
    {
        long newpos = (long)stream->buf_end + offset;
        return fseek(stream, newpos, SEEK_SET);
    }
    errno = EINVAL;
    return -1;
}
