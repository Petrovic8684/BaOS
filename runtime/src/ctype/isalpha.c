#include <ctype.h>
#ifndef EOF
#define EOF (-1)
#endif

int isalpha(int c)
{
    if (c == (int)EOF)
        return 0;
    unsigned char uc = (unsigned char)c;
    return ((uc >= 'A' && uc <= 'Z') || (uc >= 'a' && uc <= 'z'));
}
