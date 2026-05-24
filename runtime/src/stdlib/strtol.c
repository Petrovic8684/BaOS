#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

long strtol(const char *nptr, char **endptr, int base)
{
    const char *s = nptr;
    int neg = 0;
    unsigned long acc = 0;
    int any = 0;
    unsigned int c;
    unsigned long cutoff;
    unsigned int cutlim;

    while (isspace((unsigned char)*s))
        s++;

    if (*s == '+')
        s++;
    else if (*s == '-')
    {
        neg = 1;
        s++;
    }

    if (base == 0)
    {
        if (*s == '0')
        {
            if (s[1] == 'x' || s[1] == 'X')
            {
                base = 16;
                s += 2;
            }
            else
            {
                base = 8;
                s++;
            }
        }
        else
            base = 10;
    }
    else if (base == 16)
    {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            s += 2;
    }

    const char *start = s;

    if (neg)
    {
        cutoff = (unsigned long)LONG_MAX + 1UL;
    }
    else
    {
        cutoff = (unsigned long)LONG_MAX;
    }
    cutlim = (unsigned int)(cutoff % (unsigned long)base);
    unsigned long cutval = cutoff / (unsigned long)base;

    while ((c = (unsigned char)*s))
    {
        unsigned int digit;
        if (isdigit(c))
            digit = c - '0';
        else if (isalpha(c))
            digit = (unsigned int)(tolower(c) - 'a') + 10;
        else
            break;

        if (digit >= (unsigned int)base)
            break;

        if (acc > cutval || (acc == cutval && digit > cutlim))
        {
            any = 1;
            acc = (unsigned long)LONG_MAX + (neg ? 1UL : 0UL);
            while (isdigit((unsigned char)*s) || isalpha((unsigned char)*s))
                s++;
            break;
        }

        any = 1;
        acc = acc * (unsigned long)base + (unsigned long)digit;
        s++;
    }

    if (endptr != NULL)
        *endptr = (char *)(any ? s : nptr);

    if (!any)
        return 0;

    if (acc == (unsigned long)LONG_MAX + 1UL && neg)
    {
        errno = ERANGE;
        return LONG_MIN;
    }
    if (acc > (unsigned long)LONG_MAX && !neg)
    {
        errno = ERANGE;
        return LONG_MAX;
    }

    return neg ? -(long)acc : (long)acc;
}
