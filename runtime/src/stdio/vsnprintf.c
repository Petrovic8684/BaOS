#include <stdio.h>
#include "internal/syscalls.h"
#include <stdarg.h>

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
    char tmp[64];
    size_t written = 0;
    size_t outpos = 0;

#define PUTCHAR_TO_BUF(ch)                     \
    do                                         \
    {                                          \
        if (outpos + 1 < size)                 \
        {                                      \
            buf[outpos] = (char)(ch);          \
        }                                      \
        outpos += (outpos + 1 < size) ? 1 : 0; \
        written++;                             \
    } while (0)

#define PUTN_TO_BUF(ptr, n)                            \
    do                                                 \
    {                                                  \
        for (size_t __i = 0; __i < (size_t)(n); ++__i) \
        {                                              \
            if (outpos + 1 < size)                     \
                buf[outpos] = (ptr)[__i];              \
            outpos += (outpos + 1 < size) ? 1 : 0;     \
            written++;                                 \
        }                                              \
    } while (0)

    if (!buf && size != 0)
        return -1;

    for (const char *p = fmt; *p; p++)
    {
        if (*p == '%' && *(p + 1))
        {
            p++;
            int long_flag = 0;
            if (*p == 'l')
            {
                long_flag = 1;
                p++;
            }

            if (*p == 's')
            {
                const char *s = va_arg(args, const char *);
                if (!s)
                    s = "(null)";
                while (*s)
                {
                    PUTCHAR_TO_BUF(*s++);
                }
            }
            else if (*p == 'c')
            {
                char ch = (char)va_arg(args, int);
                PUTCHAR_TO_BUF(ch);
            }
            else if (*p == 'd')
            {
                long val = long_flag ? va_arg(args, long) : va_arg(args, int);
                int neg = 0;
                int i = 0;
                if (val == 0)
                {
                    PUTCHAR_TO_BUF('0');
                    continue;
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
                    PUTCHAR_TO_BUF('-');
                while (i-- > 0)
                    PUTCHAR_TO_BUF(tmp[i]);
            }
            else if (*p == 'u')
            {
                unsigned long val = long_flag ? va_arg(args, unsigned long) : va_arg(args, unsigned int);
                int i = 0;
                if (val == 0)
                {
                    PUTCHAR_TO_BUF('0');
                    continue;
                }
                while (val > 0 && i < (int)sizeof(tmp) - 1)
                {
                    tmp[i++] = '0' + (val % 10);
                    val /= 10;
                }
                while (i-- > 0)
                    PUTCHAR_TO_BUF(tmp[i]);
            }
            else if (*p == 'x' || *p == 'X')
            {
                unsigned long val = long_flag ? va_arg(args, unsigned long) : va_arg(args, unsigned int);
                int i = 0;
                const char *digits = (*p == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
                if (val == 0)
                {
                    PUTCHAR_TO_BUF('0');
                    continue;
                }
                while (val > 0 && i < (int)sizeof(tmp) - 1)
                {
                    tmp[i++] = digits[val % 16];
                    val /= 16;
                }
                while (i-- > 0)
                    PUTCHAR_TO_BUF(tmp[i]);
            }
            else if (*p == 'p')
            {
                void *ptr = va_arg(args, void *);
                unsigned long val = (unsigned long)ptr;
                PUTN_TO_BUF("0x", 2);
                int i = 0;
                const char *digits = "0123456789abcdef";
                if (val == 0)
                {
                    PUTCHAR_TO_BUF('0');
                    continue;
                }
                while (val > 0 && i < (int)sizeof(tmp) - 1)
                {
                    tmp[i++] = digits[val % 16];
                    val /= 16;
                }
                while (i-- > 0)
                    PUTCHAR_TO_BUF(tmp[i]);
            }
            else if (*p == 'f' || *p == 'g')
            {
                double val = va_arg(args, double);
                if (val < 0)
                {
                    PUTCHAR_TO_BUF('-');
                    val = -val;
                }

                long int_part = (long)val;
                double frac_part = val - int_part;
                int i = 0;
                if (int_part == 0)
                {
                    tmp[i++] = '0';
                }
                else
                {
                    long tmpval = int_part;
                    char tmpbuf[32];
                    int j = 0;
                    while (tmpval > 0 && j < (int)sizeof(tmpbuf))
                    {
                        tmpbuf[j++] = '0' + tmpval % 10;
                        tmpval /= 10;
                    }
                    while (j-- > 0)
                        tmp[i++] = tmpbuf[j];
                }
                tmp[i] = 0;
                PUTN_TO_BUF(tmp, i);

                PUTCHAR_TO_BUF('.');

                for (int d = 0; d < 10; d++)
                {
                    frac_part *= 10.0;
                    int digit = (int)frac_part;
                    PUTCHAR_TO_BUF((char)('0' + digit));
                    frac_part -= digit;
                }
            }
            else if (*p == '%')
            {
                PUTCHAR_TO_BUF('%');
            }
            else
            {
                PUTCHAR_TO_BUF('%');
                PUTCHAR_TO_BUF(*p);
            }
        }
        else
        {
            PUTCHAR_TO_BUF(*p);
        }
    }

    if (size > 0)
    {
        size_t nulpos = (outpos < size) ? outpos : (size - 1);
        buf[nulpos] = '\0';
    }

#undef PUTCHAR_TO_BUF
#undef PUTN_TO_BUF

    return (int)written;
}
