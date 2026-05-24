#include <string.h>

char *strrchr(const char *s, int c)
{
    const char *last = NULL;
    unsigned char uc = (unsigned char)c;
    for (; *s; s++)
    {
        if ((unsigned char)*s == uc)
            last = s;
    }
    if (uc == '\0')
        return (char *)s;
    return (char *)last;
}
