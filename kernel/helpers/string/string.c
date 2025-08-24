#include "string.h"

int str_equal(const char *a, const char *b)
{
    int i = 0;
    while (a[i] && b[i])
    {
        if (a[i] != b[i])
            return 0;

        i++;
    }

    return a[i] == b[i];
}

int str_count(const char *a)
{
    int count = 0;
    while (a[count])
        count++;

    return count;
}

void itoa(unsigned char num, char *str)
{
    int i = 0;
    if (num == 0)
        str[i++] = '0';
    else
        while (num > 0)
        {
            str[i++] = (num % 10) + '0';
            num /= 10;
        }

    str[i] = '\0';

    for (int j = 0; j < i / 2; j++)
    {
        char temp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = temp;
    }
}

void ftoa_fixed(double x, char *out, int precision)
{
    if (precision < 0)
        precision = 0;
    if (precision > 9)
        precision = 9;

    if (x < 0)
    {
        *out++ = '-';
        x = -x;
    }

    double round = 0.5;
    for (int i = 0; i < precision; ++i)
        round /= 10.0;
    x += round;

    int ip = (int)x;
    double frac = x - (double)ip;

    char tmp[32];
    int k = 0;
    if (ip == 0)
    {
        tmp[k++] = '0';
    }
    else
    {
        while (ip > 0 && k < (int)sizeof(tmp))
        {
            tmp[k++] = (char)('0' + (ip % 10));
            ip /= 10;
        }
    }
    for (int i = k - 1; i >= 0; --i)
        *out++ = tmp[i];

    if (precision == 0)
    {
        *out = '\0';
        return;
    }

    *out++ = '.';

    for (int i = 0; i < precision; ++i)
    {
        frac *= 10.0;
        int d = (int)frac;
        if (d < 0)
            d = 0;
        if (d > 9)
            d = 9;
        *out++ = (char)('0' + d);
        frac -= d;
    }

    char *end = out - 1;
    while (end > out - precision - 1 && *end == '0')
        end--;
    if (*end == '.')
        end--;

    *(end + 1) = '\0';
}

const char *uint_to_str(unsigned int val)
{
    static char buf[11];
    int i = 10;
    buf[i--] = '\0';

    if (val == 0)
    {
        buf[i] = '0';
        return &buf[i];
    }

    while (val > 0)
    {
        buf[i--] = '0' + (val % 10);
        val /= 10;
    }

    return &buf[i + 1];
}

void str_copy_fixed(char *dst, const char *src, unsigned int max_len)
{
    unsigned int i = 0;
    while (i < (max_len - 1) && src[i])
    {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';

    for (unsigned int j = i + 1; j < max_len; j++)
        dst[j] = '\0';
}