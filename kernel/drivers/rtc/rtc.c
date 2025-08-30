#include "rtc.h"
#include "../../helpers/bcd/bcd.h"
#include "../../helpers/ports/ports.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

static unsigned char cmos_read(unsigned char reg)
{
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

static int is_leap_year(int y)
{
    return ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
}

static long days_from_civil(int y, unsigned m, unsigned d)
{
    y -= m <= 2;
    const long era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long)doe - 719468;
}

unsigned int rtc_now(void)
{
    unsigned char sec, min, hour, day, mon, year;
    unsigned char regB;

    outb(CMOS_ADDRESS, 0x0B);
    regB = inb(CMOS_DATA);

    sec = cmos_read(0x00);
    min = cmos_read(0x02);
    hour = cmos_read(0x04);
    day = cmos_read(0x07);
    mon = cmos_read(0x08);
    year = cmos_read(0x09);

    if (!(regB & 0x04)) // BCD to binary
    {
        sec = bcd_to_bin(sec);
        min = bcd_to_bin(min);
        hour = bcd_to_bin(hour);
        day = bcd_to_bin(day);
        mon = bcd_to_bin(mon);
        year = bcd_to_bin(year);
    }

    int yr = (year < 100) ? (2000 + year) : year;

    long days = days_from_civil(yr, mon, day);
    long seconds = days * 86400 + hour * 3600 + min * 60 + sec;

    return (unsigned int)seconds;
}
