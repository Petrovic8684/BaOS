#include <stdio.h>
#include <errno.h>
#include "internal/fs_helpers.h"

#define FILE_IO_CHUNK 512u

int file_read_all(const char *pathname, void *buf, size_t max, size_t *out_size)
{
    if (!pathname || !buf || max == 0)
    {
        errno = EINVAL;
        return -1;
    }

    int size = fs_read_file_size(pathname);
    if (size < 0)
        return -1;

    if ((size_t)size >= max)
    {
        errno = EFBIG;
        return -1;
    }

    size_t total = 0;
    while (total < (size_t)size)
    {
        unsigned int got = 0;
        unsigned int chunk = (unsigned int)((size_t)size - total);
        if (chunk > FILE_IO_CHUNK)
            chunk = FILE_IO_CHUNK;

        if (fs_read_file_at(pathname, (unsigned int)total, (unsigned char *)buf + total, chunk, &got) != 0)
            return -1;

        if (got == 0)
            break;

        total += got;
    }

    if (out_size)
        *out_size = total;

    return 0;
}

int file_write_all(const char *pathname, const void *data, size_t size)
{
    if (!pathname || !data)
    {
        errno = EINVAL;
        return -1;
    }

    if (size > (size_t)0xFFFFFFFFu)
    {
        errno = EFBIG;
        return -1;
    }

    for (int attempt = 0; attempt < 2; ++attempt)
    {
        int rc = fs_write_file(pathname, (const unsigned char *)data, (unsigned int)size);
        if (rc == 0)
            return 0;
        if (attempt == 0 && errno == EIO)
            continue;
        return rc;
    }

    return -1;
}
