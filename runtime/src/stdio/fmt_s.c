#include <stdio.h>
#include "stdio/vfprintf_internal.h"

int vfprintf_fmt_s(vf_out *out, va_list *ap, int precision)
{
    int len = -1;
    if (precision >= 0)
        len = precision;

    const char *s = va_arg(*ap, const char *);
    if (!s)
        s = "(null)";

    int i = 0;
    while (s[i] && (len < 0 || i < len))
    {
        if (fputc((unsigned char)s[i], out->stream) == EOF)
            return -1;
        out->total++;
        i++;
    }

    return 0;
}
