#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include "stdio/vfprintf_internal.h"

int vfprintf(FILE *stream, const char *fmt, va_list args)
{
    vf_out out = {stream, 0};

    for (const char *p = fmt; *p; p++)
    {
        if (*p == '%' && *(p + 1))
        {
            p++;

            int long_flag = 0;
            int precision = -1;
            int width = -1;

            if (*p == '*')
            {
                width = va_arg(args, int);
                p++;
            }
            else
            {
                int seen = 0;
                int w = 0;
                while (isdigit((unsigned char)*p))
                {
                    seen = 1;
                    w = w * 10 + (*p - '0');
                    p++;
                }
                if (seen)
                    width = w;
            }

            (void)width;

            if (*p == '.')
            {
                p++;
                if (*p == '*')
                {
                    precision = va_arg(args, int);
                    p++;
                }
                else
                {
                    int seen = 0;
                    int pr = 0;
                    while (isdigit((unsigned char)*p))
                    {
                        seen = 1;
                        pr = pr * 10 + (*p - '0');
                        p++;
                    }
                    precision = seen ? pr : 0;
                }
            }

            if (*p == 'l')
            {
                long_flag = 1;
                p++;
            }

            char spec = *p;
            int rc = 0;

            if (spec == 's')
                rc = vfprintf_fmt_s(&out, &args, precision);
            else if (spec == 'c')
                rc = vfprintf_fmt_c(&out, &args);
            else if (spec == 'd')
                rc = vfprintf_fmt_d(&out, &args, long_flag);
            else if (spec == 'u')
                rc = vfprintf_fmt_u(&out, &args, long_flag);
            else if (spec == 'x' || spec == 'X')
                rc = vfprintf_fmt_x(&out, &args, long_flag, spec);
            else if (spec == 'p')
                rc = vfprintf_fmt_p(&out, &args);
            else if (spec == 'f' || spec == 'g')
                rc = vfprintf_fmt_f(&out, &args, spec);
            else if (spec == '%')
            {
                if (fputc('%', stream) == EOF)
                    return -1;
                out.total++;
            }
            else
            {
                if (fputc('%', stream) == EOF)
                    return -1;
                if (fputc(spec, stream) == EOF)
                    return -1;
                out.total += 2;
            }

            if (rc < 0)
                return -1;
        }
        else
        {
            if (fputc(*p, stream) == EOF)
                return -1;
            out.total++;
        }
    }

    return out.total;
}
