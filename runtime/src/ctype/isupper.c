#include <ctype.h>
#ifndef EOF
#define EOF (-1)
#endif

int isupper(int c)
{
    if (c == (int)EOF)
        return 0;
    unsigned char uc = (unsigned char)c;
    return (uc >= 'A' && uc <= 'Z');
}
