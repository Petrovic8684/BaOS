#include <string.h>

char *strcat(char *dest, const char *src)
{
    char *d = dest;
    while (*d)
        d++;
    while ((*d++ = *src++) != '\0')
        ;
    return dest;
}
