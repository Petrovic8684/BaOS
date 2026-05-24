#include <stdlib.h>
#include "stdlib/rand_internal.h"

unsigned int rand_seed = 1;

void srand(unsigned int seed)
{
    rand_seed = seed;
}
