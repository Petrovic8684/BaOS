#include <time.h>
#include "time/time_internal.h"

struct tm *gmtime(const time_t *timer)
{
    if (!timer)
        return NULL;
    fill_tm_from_time(*timer, time_gmtime_buf());
    return time_gmtime_buf();
}
