#include <ctype.h>
#ifndef EOF
#define EOF (-1)
#endif

int toupper(int c)
{
    if (c == (int)EOF)
        return c;
    unsigned char uc = (unsigned char)c;
    if (uc >= 'a' && uc <= 'z')
        return (int)(uc - 'a' + 'A');
    return (int)uc;
}
