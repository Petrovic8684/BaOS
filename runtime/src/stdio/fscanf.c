#include <stdio.h>
#include "internal/syscalls.h"
#include "internal/fs_helpers.h"
#include <stdarg.h>

int fscanf(FILE *stream, const char *fmt, ...)
{
    if (!stream)
        return 0;

    int file_size = fs_read_file_size(stream->name);
    if (file_size <= 0)
        return 0;

    char *buffer = (char *)stream->buf;
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
                while (buffer[pos] == ' ' || buffer[pos] == '\n' || buffer[pos] == '\t')
                    pos++;
                if (buffer[pos] == '-')
                {
                    sign = -1;
                    pos++;
                }
                while (buffer[pos] >= '0' && buffer[pos] <= '9')
                {
                    val = val * 10 + (buffer[pos] - '0');
                    pos++;
                }
                int *iptr = va_arg(args, int *);
                *iptr = val * sign;
                matched++;
            }
            else if (*p == 'c')
            {
                char *cptr = va_arg(args, char *);
                *cptr = buffer[pos++];
                matched++;
            }
            else if (*p == 's')
            {
                char *sptr = va_arg(args, char *);
                while (buffer[pos] == ' ' || buffer[pos] == '\n' || buffer[pos] == '\t')
                    pos++;
                while (buffer[pos] && buffer[pos] != ' ' && buffer[pos] != '\n' && buffer[pos] != '\t')
                {
                    *sptr++ = buffer[pos++];
                }
                *sptr = 0;
                matched++;
            }
        }
        else
        {
            if (buffer[pos++] != *p)
                break;
        }
    }

    va_end(args);
    return matched;
}
