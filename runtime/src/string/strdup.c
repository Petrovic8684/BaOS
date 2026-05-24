#include <string.h>
#include <stdlib.h>

char *strdup(const char *s)
{
    if (!s)
        return NULL;

    size_t len = strlen(s);
    char *new_str = (char *)malloc(len + 1);
    if (!new_str)
        return NULL;

    for (size_t i = 0; i <= len; i++)
        new_str[i] = s[i];

    return new_str;
}
