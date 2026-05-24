#include <stdio.h>
#include "internal/syscalls.h"
#include "internal/fs_helpers.h"
#include <errno.h>

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
        if (stream->mode == 0)
        {
            int size = fs_read_file_size(stream->name);
            if (size >= 0)
            {
                stream->buf_end = (unsigned int)size;
                if ((unsigned long)offset <= stream->buf_end)
                {
                    stream->buf_pos = (unsigned int)offset;
                    stream->eof = (stream->buf_pos >= stream->buf_end);
                    return 0;
                }
            }
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
