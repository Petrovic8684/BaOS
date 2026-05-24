#include <string.h>

char *strtok_r(char *s, const char *delim, char **saveptr)
{
    char *start;
    char *p;

    if (s == NULL)
    {
        s = *saveptr;
        if (s == NULL)
            return NULL;
    }

    for (; *s; s++)
    {
        const char *d = delim;
        int is_delim = 0;
        for (; *d; d++)
            if (*s == *d)
            {
                is_delim = 1;
                break;
            }
        if (!is_delim)
            break;
    }

    if (*s == '\0')
    {
        *saveptr = NULL;
        return NULL;
    }

    start = s;

    for (p = s; *p; p++)
    {
        const char *d = delim;
        for (; *d; d++)
            if (*p == *d)
                break;
        if (*d)
        {
            *p = '\0';
            *saveptr = p + 1;
            return start;
        }
    }

    *saveptr = NULL;
    return start;
}
