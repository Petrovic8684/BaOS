#include <stdio.h>
#include "stdio/vfprintf_internal.h"

int vfprintf_fmt_f(vf_out *out, va_list *ap, char spec)
{
    (void)spec;

    char tmp[64];
    double val = va_arg(*ap, double);

    if (val < 0)
    {
        if (fputc('-', out->stream) == EOF)
            return -1;
        out->total++;
        val = -val;
    }

    long int_part = (long)val;
    double frac_part = val - int_part;
    int i = 0;

    if (int_part == 0)
        tmp[i++] = '0';
    else
    {
        long tmpval = int_part;
        char tmpbuf[32];
        int j = 0;
        while (tmpval > 0)
        {
            tmpbuf[j++] = '0' + tmpval % 10;
            tmpval /= 10;
        }
        while (j-- > 0)
            tmp[i++] = tmpbuf[j];
    }

    tmp[i] = 0;
    if (fputs(tmp, out->stream) == EOF)
        return -1;
    out->total += i;

    if (fputc('.', out->stream) == EOF)
        return -1;
    out->total++;

    for (int d = 0; d < 10; d++)
    {
        frac_part *= 10;
        int digit = (int)frac_part;
        if (fputc('0' + digit, out->stream) == EOF)
            return -1;
        out->total++;
        frac_part -= digit;
    }

    return 0;
}
