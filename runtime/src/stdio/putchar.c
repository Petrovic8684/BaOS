#include <stdio.h>
#include "internal/syscalls.h"

int putchar(int c) { return fputc(c, stdout); }
