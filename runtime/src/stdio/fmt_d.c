#include <stdio.h>
#include "stdio/vfprintf_internal.h"

int vfprintf_fmt_d(vf_out *out, va_list *ap, int long_flag)
{
    char tmp[64];
    long val = long_flag ? va_arg(*ap, long) : va_arg(*ap, int);
    int neg = 0;
    int i = 0;

    if (val == 0)
    {
        if (fputc('0', out->stream) == EOF)
            return -1;
        out->total++;
        return 0;
    }

    if (val < 0)
    {
        neg = 1;
        val = -val;
    }

    while (val > 0 && i < (int)sizeof(tmp) - 1)
    {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }

    if (neg)
    {
        if (fputc('-', out->stream) == EOF)
            return -1;
        out->total++;
    }

    while (i-- > 0)
    {
        if (fputc(tmp[i], out->stream) == EOF)
            return -1;
        out->total++;
    }

    return 0;
}
