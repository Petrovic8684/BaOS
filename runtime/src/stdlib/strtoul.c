#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

unsigned long strtoul(const char *nptr, char **endptr, int base)
{
    const char *s = nptr;
    unsigned long acc = 0;
    int c;
    int neg = 0;

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

    while ((c = (unsigned char)*s))
    {
        unsigned int digit;

        if (isdigit(c))
            digit = c - '0';
        else if (isalpha(c))
            digit = (tolower(c) - 'a') + 10;
        else
            break;

        if (digit >= (unsigned int)base)
            break;

        if (acc > (ULONG_MAX - digit) / base)
        {
            acc = ULONG_MAX;
            errno = ERANGE;
            while (isdigit((unsigned char)*s) || isalpha((unsigned char)*s))
                s++;
            break;
        }

        acc = acc * base + digit;
        s++;
    }

    if (endptr != NULL)
        *endptr = (char *)(s != start ? s : nptr);

    return neg ? (unsigned long)(-(long)acc) : acc;
}
