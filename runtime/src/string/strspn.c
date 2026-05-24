#include <string.h>

size_t strspn(const char *s, const char *accept)
{
    size_t count = 0;
    const char *p;
    for (; *s; s++)
    {
        for (p = accept; *p; p++)
        {
            if (*s == *p)
                break;
        }
        if (*p == '\0')
            break;
        count++;
    }
    return count;
}
