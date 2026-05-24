#include <string.h>

int strncmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0)
        return 0;
    const unsigned char *a = (const unsigned char *)s1;
    const unsigned char *b = (const unsigned char *)s2;
    while (n--)
    {
        if (*a != *b)
            return (int)(*a - *b);
        if (*a == '\0')
            return 0;
        a++;
        b++;
    }
    return 0;
}
