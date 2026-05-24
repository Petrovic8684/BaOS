#include <string.h>

char *strtok(char *s, const char *delim)
{
    static char *saveptr;
    return strtok_r(s, delim, &saveptr);
}
