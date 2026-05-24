#include <stdio.h>
#include "stdio/vfprintf_internal.h"

int vfprintf_fmt_p(vf_out *out, va_list *ap)
{
    char tmp[64];
    void *ptr = va_arg(*ap, void *);
    unsigned long val = (unsigned long)ptr;
    int i = 0;
    const char *digits = "0123456789abcdef";

    if (fputs("0x", out->stream) == EOF)
        return -1;
    out->total += 2;

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
