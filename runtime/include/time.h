#ifndef TIME_H
#define TIME_H

typedef long time_t;

struct tm
{
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
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
