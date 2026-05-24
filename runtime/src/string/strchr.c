#include <string.h>

char *strchr(const char *s, int c)
{
    unsigned char uc = (unsigned char)c;
    for (;; s++)
    {
        if ((unsigned char)*s == uc)
            return (char *)s;
        if (*s == '\0')
            break;
    }
    return NULL;
}
