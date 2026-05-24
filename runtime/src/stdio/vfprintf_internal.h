#ifndef BAOS_STDIO_VFPRINTF_INTERNAL_H
#define BAOS_STDIO_VFPRINTF_INTERNAL_H

#include <stdio.h>
#include <stdarg.h>

typedef struct vf_out
{
    FILE *stream;
    int total;
} vf_out;

int vfprintf_fmt_s(vf_out *out, va_list *ap, int precision);
int vfprintf_fmt_c(vf_out *out, va_list *ap);
int vfprintf_fmt_d(vf_out *out, va_list *ap, int long_flag);
int vfprintf_fmt_u(vf_out *out, va_list *ap, int long_flag);
int vfprintf_fmt_x(vf_out *out, va_list *ap, int long_flag, char spec);
int vfprintf_fmt_p(vf_out *out, va_list *ap);
int vfprintf_fmt_f(vf_out *out, va_list *ap, char spec);

#endif
