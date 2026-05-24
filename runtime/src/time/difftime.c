#include <time.h>
#include "internal/syscalls.h"

double difftime(time_t time1, time_t time0)
{
    return (double)(time1 - time0);
}
