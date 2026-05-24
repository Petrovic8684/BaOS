#include <ctype.h>
#ifndef EOF
#define EOF (-1)
#endif

int ispunct(int c)
{
    if (c == (int)EOF)
        return 0;
    unsigned char uc = (unsigned char)c;
    if (uc <= 0x20 || uc >= 0x7f)
        return 0;
    if ((uc >= '0' && uc <= '9') || (uc >= 'A' && uc <= 'Z') || (uc >= 'a' && uc <= 'z') || uc == ' ')
        return 0;
    return 1;
}
