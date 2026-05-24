#include <stdio.h>
#include <errno.h>



void perror(const char *s)
{
    const char *msg = strerror(errno);
    if (s && s[0] != '\0')
        fprintf(stderr, "%s: %s\n", s, msg);
    else
        fprintf(stderr, "%s\n", msg);
}
