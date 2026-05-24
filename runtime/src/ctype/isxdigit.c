#include <ctype.h>
#ifndef EOF
#define EOF (-1)
#endif

int isxdigit(int c)
{
    if (c == (int)EOF)
        return 0;
    unsigned char uc = (unsigned char)c;
    return ((uc >= '0' && uc <= '9') || (uc >= 'A' && uc <= 'F') || (uc >= 'a' && uc <= 'f'));
}
