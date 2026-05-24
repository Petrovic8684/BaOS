#include <string.h>

char *strcpy(char *dest, const char *src)
{
    char *d = dest;
    const char *s = src;
    while (*s)
        *d++ = *s++;

    *d = '\0';
    return dest;
}
