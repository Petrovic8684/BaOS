#ifndef STDARG_H
#define STDARG_H

typedef char *va_list;

#define _VA_ROUND_SIZEOF(type) ((sizeof(type) + sizeof(void *) - 1) & ~(sizeof(void *) - 1))
#define va_start(ap, last) (ap = (char *)&(last) + _VA_ROUND_SIZEOF(last))
#define va_arg(ap, type) (*(type *)((ap += _VA_ROUND_SIZEOF(type)) - _VA_ROUND_SIZEOF(type)))
#define va_end(ap) (ap = (va_list)0)
#define va_copy(dest, src) (dest = src)

#endif
