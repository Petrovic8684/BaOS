#include <string.h>

size_t strxfrm(char *dest, const char *src, size_t n)
{
    size_t len = strlen(src);

    if (n > 0)
    {
        size_t copy = (len < n - 1) ? len : (n - 1);
        for (size_t i = 0; i < copy; i++)
            dest[i] = src[i];

        dest[copy] = '\0';
    }

    return len;
}
