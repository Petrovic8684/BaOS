#include <time.h>
#include <string.h>
#include <stdio.h>

#define SYS_RTC_NOW 5
#define TIMEZONE_FILE_PATH "/config/timezone"

static short timezone_offset = 0;

static int is_leap(int y)
{
    return ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
}

static void civil_from_days(int z, int *y, int *m, int *d)
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

static struct tm gmtime_buf;
static struct tm localtime_buf;
static char asctime_buf[26];

static void fill_tm_from_time(time_t t, struct tm *out)
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
    static const int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
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

static time_t time_from_tm(const struct tm *tm)
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

time_t time(time_t *t)
{
    time_t secs;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[secs]\n\t"
        : [secs] "=r"(secs)
        : [num] "i"(SYS_RTC_NOW)
        : "eax", "ebx", "memory");

    if (t)
        *t = secs;
    return secs;
}

struct tm *gmtime(const time_t *timer)
{
    if (!timer)
        return NULL;
    fill_tm_from_time(*timer, &gmtime_buf);
    return &gmtime_buf;
}

static int read_timezone_offset(void)
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

struct tm *localtime(const time_t *timer)
{
    if (!timer)
        return NULL;

    read_timezone_offset();

    time_t t = *timer + timezone_offset * 3600;
    fill_tm_from_time(t, &localtime_buf);
    return &localtime_buf;
}

time_t mktime(struct tm *tm)
{
    if (!tm)
        return (time_t)-1;
    return time_from_tm(tm);
}

char *asctime(const struct tm *tm)
{
    static const char *wday_name[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char *mon_name[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
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

char *ctime(const time_t *timer)
{
    if (!timer)
        return NULL;
    struct tm *tm = localtime(timer);
    return asctime(tm);
}

double difftime(time_t time1, time_t time0)
{
    return (double)(time1 - time0);
}
