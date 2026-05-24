#include <stdio.h>
#include "internal/syscalls.h"
#include <stdarg.h>

int sscanf(const char *str, const char *fmt, ...)
{
    int pos = 0;
    va_list args;
    va_start(args, fmt);
    int matched = 0;

    for (const char *p = fmt; *p; p++)
    {
        if (*p == '%')
        {
            p++;
            if (*p == 'd')
            {
                int sign = 1, val = 0;
                while (str[pos] == ' ' || str[pos] == '\n' || str[pos] == '\t')
                    pos++;
                if (str[pos] == '-')
                {
                    sign = -1;
                    pos++;
                }
                while (str[pos] >= '0' && str[pos] <= '9')
                {
                    val = val * 10 + (str[pos] - '0');
                    pos++;
                }
                int *iptr = va_arg(args, int *);
                *iptr = val * sign;
                matched++;
            }
            else if (*p == 'c')
            {
                char *cptr = va_arg(args, char *);
                *cptr = str[pos++];
                matched++;
            }
            else if (*p == 's')
            {
                char *sptr = va_arg(args, char *);
                while (str[pos] == ' ' || str[pos] == '\n' || str[pos] == '\t')
                    pos++;
                while (str[pos] && str[pos] != ' ' && str[pos] != '\n' && str[pos] != '\t')
                {
                    *sptr++ = str[pos++];
                }
                *sptr = 0;
                matched++;
            }
        }
        else
        {
            if (str[pos++] != *p)
                break;
        }
    }

    va_end(args);
    return matched;
}
