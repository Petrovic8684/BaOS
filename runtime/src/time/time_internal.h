#ifndef BAOS_TIME_INTERNAL_H
#define BAOS_TIME_INTERNAL_H

#include <time.h>

#define TIMEZONE_FILE_PATH "/config/timezone"

void fill_tm_from_time(time_t t, struct tm *out);
time_t time_from_tm(const struct tm *tm);
int read_timezone_offset(void);
struct tm *time_gmtime_buf(void);
struct tm *time_localtime_buf(void);
char *time_asctime_buf(void);
short *time_timezone_offset(void);

#endif
