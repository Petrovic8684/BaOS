#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>

#define SYS_EXIT 0

void exit(int code)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[c], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_EXIT), [c] "r"(code)
        : "eax", "ebx", "memory");
}

double strtod(const char *nptr, char **endptr)
{
    const char *s = nptr;
    double result = 0.0;
    int sign = 1;
    int exp_sign = 1;
    long exponent = 0;
    double frac = 0.0;
    double frac_div = 1.0;

    while (isspace((unsigned char)*s))
        s++;

    if (*s == '+')
        s++;
    else if (*s == '-')
    {
        sign = -1;
        s++;
    }

    while (isdigit((unsigned char)*s))
    {
        result = result * 10.0 + (*s - '0');
        s++;
    }

    if (*s == '.')
    {
        s++;
        while (isdigit((unsigned char)*s))
        {
            frac = frac * 10.0 + (*s - '0');
            frac_div *= 10.0;
            s++;
        }
        result += frac / frac_div;
    }

    if (*s == 'e' || *s == 'E')
    {
        s++;
        if (*s == '+')
            s++;
        else if (*s == '-')
        {
            exp_sign = -1;
            s++;
        }
        while (isdigit((unsigned char)*s))
        {
            exponent = exponent * 10 + (*s - '0');
            s++;
        }
    }

    if (exponent != 0)
    {
        double pow10 = pow(10.0, exp_sign * exponent);
        result *= pow10;
    }

    if (endptr != NULL)
        *endptr = (char *)s;

    return sign * result;
}

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