#include <string.h>

size_t strcspn(const char *s, const char *reject)
{
    size_t count = 0;
    const char *p;
    for (; *s; s++)
    {
        for (p = reject; *p; p++)
        {
            if (*s == *p)
                return count;
        }
        count++;
    }
    return count;
}
