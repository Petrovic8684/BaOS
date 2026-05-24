#include <time.h>
#include "time/time_internal.h"

char *ctime(const time_t *timer)
{
    if (!timer)
        return NULL;
    struct tm *tm = localtime(timer);
    return asctime(tm);
}
