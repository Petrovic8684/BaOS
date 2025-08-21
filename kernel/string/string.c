#include "string.h"

// Compare strings
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

// String length
int str_count(const char *a)
{
    int count = 0;
    while (a[count])
        count++;
    return count;
}

// Cast unsigned char to string
void itoa(unsigned char num, char *str)
{
    int i = 0;
    if (num == 0)
    {
        str[i++] = '0';
    }
    else
    {
        while (num > 0)
        {
            str[i++] = (num % 10) + '0';
            num /= 10;
        }
    }
    str[i] = '\0';

    // Reverse the string
    for (int j = 0; j < i / 2; j++)
    {
        char temp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = temp;
    }
}