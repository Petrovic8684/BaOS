#include <time.h>
#include "time/time_internal.h"

char *asctime(const struct tm *tm)
{
    static const char *wday_name[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char *mon_name[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    char *asctime_buf = time_asctime_buf();
    if (!tm)
        return NULL;

    int y = tm->tm_year + 1900;
    int month = tm->tm_mon;
    int day = tm->tm_mday;
    int hour = tm->tm_hour;
    int min = tm->tm_min;
    int sec = tm->tm_sec;
    int wday = tm->tm_wday;

    const char *wd = (wday >= 0 && wday < 7) ? wday_name[wday] : "Day";
    const char *mn = (month >= 0 && month < 12) ? mon_name[month] : "Mon";

    int pos = 0;
    for (const char *p = wd; *p; ++p)
        asctime_buf[pos++] = *p;
    asctime_buf[pos++] = ' ';
    for (const char *p = mn; *p; ++p)
        asctime_buf[pos++] = *p;
    asctime_buf[pos++] = ' ';
    if (day < 10)
    {
        asctime_buf[pos++] = ' ';
        asctime_buf[pos++] = '0' + day;
    }
    else
    {
        asctime_buf[pos++] = '0' + (day / 10);
        asctime_buf[pos++] = '0' + (day % 10);
    }
    asctime_buf[pos++] = ' ';
    asctime_buf[pos++] = '0' + ((hour / 10) % 10);
    asctime_buf[pos++] = '0' + (hour % 10);
    asctime_buf[pos++] = ':';
    asctime_buf[pos++] = '0' + ((min / 10) % 10);
    asctime_buf[pos++] = '0' + (min % 10);
    asctime_buf[pos++] = ':';
    asctime_buf[pos++] = '0' + ((sec / 10) % 10);
    asctime_buf[pos++] = '0' + (sec % 10);
    asctime_buf[pos++] = ' ';
    int y1000 = y / 1000, y100 = (y / 100) % 10, y10 = (y / 10) % 10, y1 = y % 10;
    asctime_buf[pos++] = '0' + y1000;
    asctime_buf[pos++] = '0' + y100;
    asctime_buf[pos++] = '0' + y10;
    asctime_buf[pos++] = '0' + y1;
    asctime_buf[pos++] = '\n';
    asctime_buf[pos] = '\0';
    return asctime_buf;
}
