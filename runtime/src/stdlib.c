#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

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