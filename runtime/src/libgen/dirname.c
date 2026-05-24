#include <libgen.h>
#include <string.h>
#include <stdlib.h>

char *dirname(const char *path)
{
    if (!path || *path == '\0')
        return strdup(".");

    const char *end = path + strlen(path);

    while (end > path && *(end - 1) == '/')
        end--;

    if (end == path)
        return strdup("/");

    const char *slash = end;
    while (slash > path && *(slash - 1) != '/')
        slash--;

    if (slash == path)
        return strdup(".");

    while (slash > path && *(slash - 1) == '/')
        slash--;

    if (slash == path)
        return strdup("/");

    size_t len = (size_t)(slash - path);
    char *out = (char *)malloc(len + 1);
    if (!out)
        return NULL;
    memcpy(out, path, len);
    out[len] = '\0';

    return out;
}
