#include <stdlib.h>
#include "stdlib/rand_internal.h"

int rand(void)
{
    rand_seed = rand_seed * 1103515245 + 12345;
    return (int)((rand_seed >> 16) & 0x7FFF);
}
