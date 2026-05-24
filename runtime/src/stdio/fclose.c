#include <stdio.h>
#include "internal/syscalls.h"
#include <errno.h>
#include "stdio/file_internal.h"

int fclose(FILE *stream)
{
    if (!stream)
    {
        errno = EINVAL;
        return EOF;
    }

    if (stream == stdout || stream == stderr || stream == stdin)
    {
        errno = EINVAL;
        return EOF;
    }

    if (stream->mode == 1 && stream->buf_pos > 0)
        fflush(stream);

    free_file_slot(stream);
    return 0;
}
