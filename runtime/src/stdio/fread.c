#include <stdio.h>
#include "internal/syscalls.h"
#include <errno.h>

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    if (!ptr || !stream || size == 0 || nmemb == 0)
    {
        errno = EINVAL;
        return 0;
    }
    size_t total = size * nmemb;
    size_t i;
    unsigned char *out = (unsigned char *)ptr;
    for (i = 0; i < total; i++)
    {
        int c = fgetc(stream);
        if (c == EOF)
        {
            errno = EIO;
            break;
        }
        out[i] = (unsigned char)c;
    }
    return i / size;
}
