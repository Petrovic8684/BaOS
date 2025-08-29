#ifndef TIME_H
#define TIME_H

#include <stddef.h>

typedef long time_t;

struct tm
{
    int tm_sec;   /* seconds [0,61] */
    int tm_min;   /* minutes [0,59] */
    int tm_hour;  /* hour [0,23] */
    int tm_mday;  /* day of month [1,31] */
    int tm_mon;   /* months since January [0,11] */
    int tm_year;  /* years since 1900 */
    int tm_wday;  /* days since Sunday [0,6] */
    int tm_yday;  /* days since January 1 [0,365] */
    int tm_isdst; /* daylight saving time flag */
};

#define CLOCKS_PER_SEC 1000000L

time_t time(time_t *t);
struct tm *gmtime(const time_t *timer);
struct tm *localtime(const time_t *timer);
time_t mktime(struct tm *tm);
char *asctime(const struct tm *tm);
char *ctime(const time_t *timer);
double difftime(time_t time1, time_t time0);

#endif
