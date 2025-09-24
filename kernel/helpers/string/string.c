#include "string.h"

int str_equal(const char *a, const char *b)
{
    int i = 0;
    while (a[i] && b[i])
    {
        if (a[i] != b[i])
            return 0;

        i++;
    }

    return a[i] == b[i];
}

int str_count(const char *a)
{
    int count = 0;
    while (a[count])
        count++;

    return count;
}

void str_copy_fixed(char *dst, const char *src, unsigned int max_len)
{
    unsigned int i = 0;
    while (i < (max_len - 1) && src[i])
    {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';

    for (unsigned int j = i + 1; j < max_len; j++)
        dst[j] = '\0';
}