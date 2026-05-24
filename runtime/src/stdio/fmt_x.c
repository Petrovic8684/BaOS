#include <stdio.h>
#include "stdio/vfprintf_internal.h"

int vfprintf_fmt_x(vf_out *out, va_list *ap, int long_flag, char spec)
{
    char tmp[64];
    unsigned long val = long_flag ? va_arg(*ap, unsigned long) : va_arg(*ap, unsigned int);
    int i = 0;
    const char *digits = (spec == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";

    if (val == 0)
    {
        if (fputc('0', out->stream) == EOF)
            return -1;
        out->total++;
        return 0;
    }

    while (val > 0 && i < (int)sizeof(tmp) - 1)
    {
        tmp[i++] = digits[val % 16];
        val /= 16;
    }

    while (i-- > 0)
    {
        if (fputc(tmp[i], out->stream) == EOF)
            return -1;
        out->total++;
    }

    return 0;
}
