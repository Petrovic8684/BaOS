#include <stdio.h>
#include "internal/syscalls.h"
#include <errno.h>

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    if (!ptr || !stream || size == 0 || nmemb == 0)
    {
        errno = EINVAL;
        return 0;
    }

    size_t total = size * nmemb;
    const unsigned char *in = (const unsigned char *)ptr;
    size_t i;
    for (i = 0; i < total; i++)
        if (fputc(in[i], stream) == EOF)
        {
            errno = EIO;
            break;
        }

    return i / size;
}
