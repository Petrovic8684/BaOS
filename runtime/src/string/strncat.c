#include <string.h>

char *strncat(char *dest, const char *src, size_t n)
{
    char *d = dest;
    while (*d)
        d++;
    while (n-- && *src)
    {
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}
