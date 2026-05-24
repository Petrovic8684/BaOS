#include <ctype.h>
#ifndef EOF
#define EOF (-1)
#endif

int isspace(int c)
{
    if (c == (int)EOF)
        return 0;
    unsigned char uc = (unsigned char)c;
    return (uc == ' ' || uc == '\f' || uc == '\n' || uc == '\r' || uc == '\t' || uc == '\v');
}
