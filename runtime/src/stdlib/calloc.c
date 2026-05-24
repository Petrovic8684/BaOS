#include <stdlib.h>
#include <string.h>
#include <errno.h>

void *calloc(unsigned int nmemb, unsigned int size)
{
    if (nmemb == 0 || size == 0)
        return malloc(0);

    if (size != 0 && nmemb > (unsigned int)(~0u) / size)
    {
        errno = ENOMEM;
        return NULL;
    }

    unsigned int total = nmemb * size;
    void *p = malloc(total);
    if (!p)
        return NULL;

    memset(p, 0, total);
    return p;
}
