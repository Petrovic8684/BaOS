#include <time.h>
#include "internal/syscalls.h"
#include "time/time_internal.h"

time_t mktime(struct tm *tm)
{
    if (!tm)
        return (time_t)-1;
    return time_from_tm(tm);
}
