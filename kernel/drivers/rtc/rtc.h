#ifndef RTC_H
#define RTC_H

#include "../../helpers/bcd/bcd.h"
#include "../../helpers/ports/ports.h"

typedef struct
{
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned char year;
} DateTime;

DateTime rtc_now(void);

#endif
