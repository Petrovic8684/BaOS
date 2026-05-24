#include <libgen.h>
#include <string.h>
#include <stdlib.h>

char *basename(const char *path)
{
    if (!path || *path == '\0')
        return strdup(".");

    const char *end = path + strlen(path);

    while (end > path && *(end - 1) == '/')
        end--;

    if (end == path)
        return strdup("/");

    const char *start = end;
    while (start > path && *(start - 1) != '/')
        start--;

    size_t len = (size_t)(end - start);
    char *out = (char *)malloc(len + 1);
    if (!out)
        return NULL;
    memcpy(out, start, len);
    out[len] = '\0';

    return out;
}
