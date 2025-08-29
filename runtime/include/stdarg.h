#ifndef STDARG_H
#define STDARG_H

#include <stddef.h>

typedef char *va_list;

#define _VA_ROUND_SIZEOF(type) \
    ((sizeof(type) + sizeof(uintptr_t) - 1) & ~(sizeof(uintptr_t) - 1))

#define va_start(ap, last) \
    (ap = (char *)&(last) + _VA_ROUND_SIZEOF(last))

#define va_arg(ap, type) \
    (*(type *)((ap += _VA_ROUND_SIZEOF(type)) - _VA_ROUND_SIZEOF(type)))

#define va_end(ap) (ap = (va_list)0)

#ifndef va_copy
#define va_copy(dest, src) (dest = src)
#endif

#endif
