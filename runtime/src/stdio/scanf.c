#include <stdio.h>
#include "internal/syscalls.h"
#include <stdlib.h>
#include <stdarg.h>

int scanf(const char *fmt, ...)
{
    char *line = NULL;
    read_line(&line);
    if (!line)
        return 0;

    va_list args;
    va_start(args, fmt);
    int matched = 0;

    char *cursor = line;

    for (const char *p = fmt; *p; p++)
    {
        if (*p == '%')
        {
            p++;
            if (*p == 'd')
            {
                int val;
                int n = 0;
                if (sscanf(cursor, "%d%n", &val, &n) == 1)
                {
                    int *iptr = va_arg(args, int *);
                    *iptr = val;
                    matched++;
                    cursor += n;
                }
                else
                {
                    break;
                }
            }
            else if (*p == 'c')
            {
                char *cptr = va_arg(args, char *);
                if (*cursor)
                {
                    *cptr = *cursor;
                    matched++;
                    cursor++;
                }
                else
                {
                    break;
                }
            }
            else if (*p == 's')
            {
                char *sptr = va_arg(args, char *);
                int n = 0;
                if (sscanf(cursor, "%s%n", sptr, &n) == 1)
                {
                    matched++;
                    cursor += n;
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            if (*cursor != *p)
                break;
            cursor++;
        }

        while (*cursor == ' ' || *cursor == '\t')
            cursor++;
    }

    va_end(args);
    free(line);
    return matched;
}
