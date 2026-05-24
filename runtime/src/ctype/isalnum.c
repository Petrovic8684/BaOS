#include <ctype.h>
#ifndef EOF
#define EOF (-1)
#endif

int isalnum(int c)
{
    if (c == (int)EOF)
        return 0;
    unsigned char uc = (unsigned char)c;
    return ((uc >= '0' && uc <= '9') || (uc >= 'A' && uc <= 'Z') || (uc >= 'a' && uc <= 'z'));
}
