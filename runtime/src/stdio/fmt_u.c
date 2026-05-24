#include <stdio.h>
#include "stdio/vfprintf_internal.h"

int vfprintf_fmt_u(vf_out *out, va_list *ap, int long_flag)
{
    char tmp[64];
    unsigned long val = long_flag ? va_arg(*ap, unsigned long) : va_arg(*ap, unsigned int);
    int i = 0;

    if (val == 0)
    {
        if (fputc('0', out->stream) == EOF)
            return -1;
        out->total++;
        return 0;
    }

    while (val > 0 && i < (int)sizeof(tmp) - 1)
    {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }

    while (i-- > 0)
    {
        if (fputc(tmp[i], out->stream) == EOF)
            return -1;
        out->total++;
    }

    return 0;
}
