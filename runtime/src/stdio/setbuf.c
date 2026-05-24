#include <stdio.h>
#include "internal/syscalls.h"

void setbuf(FILE *stream, char *buf)
{
    (void)stream;
    (void)buf;
}
