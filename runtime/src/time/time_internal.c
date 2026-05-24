#include "time/time_internal.h"
#include <stdio.h>
#include <string.h>

static short timezone_offset = 0;

static struct tm gmtime_buf;

static struct tm localtime_buf;

static char asctime_buf[26];



int is_leap(int y)
{
    return ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
}



void civil_from_days(int z, int *y, int *m, int *d)
{
    z += 719468;
    int era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned doe = (unsigned)(z - era * 146097);
    unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int yy = (int)(yoe + era * 400);
    unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    unsigned mp = (5 * doy + 2) / 153;
    unsigned day = doy - (153 * mp + 2) / 5 + 1;
    unsigned month = mp + (mp < 10 ? 3 : -9);
    yy += (month <= 2);
    *y = yy;
    *m = (int)month;
    *d = (int)day;
}



void fill_tm_from_time(time_t t, struct tm *out)
{
    int days = t / 86400;
    int rem = t % 86400;
    if (rem < 0)
    {
        rem += 86400;
        days -= 1;
    }

    int y, m, d;
    civil_from_days(days, &y, &m, &d);

    int hour = rem / 3600;
    int min = (rem % 3600) / 60;
    int sec = rem % 60;

    int day_of_year = 0;
    const int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    for (int i = 1; i < m; ++i)
    {
        day_of_year += mdays[i - 1];
        if (i == 2 && is_leap(y))
            day_of_year += 1;
    }
    day_of_year += d - 1;

    int wday = (days + 4) % 7;
    if (wday < 0)
        wday += 7;

    out->tm_sec = sec;
    out->tm_min = min;
    out->tm_hour = hour;
    out->tm_mday = d;
    out->tm_mon = m - 1;
    out->tm_year = y - 1900;
    out->tm_wday = wday;
    out->tm_yday = day_of_year;
    out->tm_isdst = 0;
}



time_t time_from_tm(const struct tm *tm)
{
    int year = tm->tm_year + 1900;
    unsigned mon = (unsigned)(tm->tm_mon + 1);
    unsigned day = (unsigned)tm->tm_mday;

    int days = 0;
    {
        int y = year;
        int m = mon;
        int d = day;
        y -= m <= 2;
        int era = (y >= 0 ? y : y - 399) / 400;
        unsigned yoe = (unsigned)(y - era * 400);
        unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
        unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        days = era * 146097 + (int)doe - 719468;
    }

    return days * 86400 + tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;
}



int read_timezone_offset(void)
{
    FILE *f = fopen(TIMEZONE_FILE_PATH, "r");
    if (!f)
    {
        printf("\033[31mError: Could not read timezone. Displaying UTC time...\033[0m\n");
        return -1;
    }

    int offset;
    int read = fscanf(f, "%d\n", &offset);
    fclose(f);

    if (read <= 0)
    {
        printf("\033[31mError: Could not read timezone. Displaying UTC time...\033[0m\n");
        return -1;
    }

    if (offset < -12 || offset > 14)
    {
        printf("\033[31mError: Current timezone is invalid. Displaying UTC time...\033[0m\n");
        return -1;
    }

    timezone_offset = offset;
    return 0;
}

struct tm *time_gmtime_buf(void) { return &gmtime_buf; }
struct tm *time_localtime_buf(void) { return &localtime_buf; }
char *time_asctime_buf(void) { return asctime_buf; }
short *time_timezone_offset(void) { return &timezone_offset; }
