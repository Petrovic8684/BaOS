#include <string.h>

char *strpbrk(const char *s, const char *accept)
{
    const char *p;
    for (; *s; s++)
    {
        for (p = accept; *p; p++)
        {
            if (*s == *p)
                return (char *)s;
        }
    }
    return NULL;
}
