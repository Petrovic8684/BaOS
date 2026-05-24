#include <stdlib.h>
#include <string.h>

static void qsort_recursive(char *base, size_t nmemb, size_t size,
                            int (*compar)(const void *, const void *))
{
    if (nmemb < 2)
        return;

    char *pivot = base + (nmemb / 2) * size;
    char *left = base;
    char *right = base + (nmemb - 1) * size;
    char tmp[size];

    while (left <= right)
    {
        while (compar(left, pivot) < 0)
            left += size;
        while (compar(right, pivot) > 0)
            right -= size;
        if (left <= right)
        {
            memcpy(tmp, left, size);
            memcpy(left, right, size);
            memcpy(right, tmp, size);
            left += size;
            right -= size;
        }
    }

    size_t left_count = (right - base) / size + 1;
    size_t right_count = nmemb - ((left - base) / size);

    if (left_count > 0)
        qsort_recursive(base, left_count, size, compar);
    if (right_count > 0)
        qsort_recursive(left, right_count, size, compar);
}

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *))
{
    qsort_recursive((char *)base, nmemb, size, compar);
}
