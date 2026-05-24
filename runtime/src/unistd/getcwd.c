#include <unistd.h>
#include "internal/fs_helpers.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

char *getcwd(char *buf, size_t size)
{
    char *path = fs_where();
    if (!path)
        return NULL;

    size_t len = strlen(path);

    char *retbuf = buf;
    if (!buf)
    {
        retbuf = malloc(len + 1);
        if (!retbuf)
        {
            free(path);
            errno = ENOMEM;
            return NULL;
        }
    }
    else if (len + 1 > size)
    {
        free(path);
        errno = ERANGE;
        return NULL;
    }

    memcpy(retbuf, path, len + 1);
    free(path);

    return retbuf;
}
