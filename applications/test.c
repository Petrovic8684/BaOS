#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    printf("=== allocator test start ===\n");

    // malloc
    char *p = malloc(16);
    if (!p)
    {
        printf("malloc failed\n");
        return 1;
    }
    strcpy(p, "hello malloc");
    printf("malloc -> %s\n", p);

    // calloc
    int *arr = calloc(10, sizeof(int));
    if (!arr)
    {
        printf("calloc failed\n");
        free(p);
        return 1;
    }
    int zeros = 1;
    for (int i = 0; i < 10; i++)
    {
        if (arr[i] != 0)
            zeros = 0;
    }
    printf("calloc zeroed? %s\n", zeros ? "yes" : "no");

    // realloc
    char *q = realloc(p, 32);
    if (!q)
    {
        printf("realloc failed\n");
        free(arr);
        return 1;
    }
    strcat(q, " + more");
    printf("realloc -> %s\n", q);

    // free
    free(q);
    free(arr);

    printf("=== allocator test end ===\n");
    return 0;
}
