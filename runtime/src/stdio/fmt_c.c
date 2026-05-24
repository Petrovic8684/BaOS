#include <stdio.h>
#include "stdio/vfprintf_internal.h"

int vfprintf_fmt_c(vf_out *out, va_list *ap)
{
    char ch = (char)va_arg(*ap, int);
    if (fputc(ch, out->stream) == EOF)
        return -1;
    out->total++;
    return 0;
}
