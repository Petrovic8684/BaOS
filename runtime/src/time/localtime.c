#include <time.h>
#include "time/time_internal.h"

struct tm *localtime(const time_t *timer)
{
    if (!timer)
        return NULL;

    read_timezone_offset();

    time_t t = *timer + (*time_timezone_offset()) * 3600;
    fill_tm_from_time(t, time_localtime_buf());
    return time_localtime_buf();
}
