#include "system.h"

// CMOS ports
#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

#define TIMEZONE_OFFSET 2 // UTC+2

#define OS_NAME "BaOS (Jovan Petrovic, 2025)"
#define KERNEL_VERSION "1.0"

static unsigned char cmos_read(unsigned char reg)
{
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

static void apply_timezone(DateTime *dt)
{
    int hour = dt->hour + TIMEZONE_OFFSET;
    if (hour >= 24)
    {
        hour -= 24;
        dt->day++;
        if (dt->day > 31)
        {
            dt->day = 1;
            dt->month++;
            if (dt->month > 12)
            {
                dt->month = 1;
                dt->year++;
            }
        }
    }
    dt->hour = hour;
}

DateTime date(void)
{
    DateTime dt;

    unsigned char regB;
    outb(CMOS_ADDRESS, 0x0B);
    regB = inb(CMOS_DATA);

    dt.second = cmos_read(0x00);
    dt.minute = cmos_read(0x02);
    dt.hour = cmos_read(0x04);
    dt.day = cmos_read(0x07);
    dt.month = cmos_read(0x08);
    dt.year = cmos_read(0x09);

    // Convert from BCD
    if (!(regB & 0x04))
    {
        dt.second = bcd_to_bin(dt.second);
        dt.minute = bcd_to_bin(dt.minute);
        dt.hour = bcd_to_bin(dt.hour);
        dt.day = bcd_to_bin(dt.day);
        dt.month = bcd_to_bin(dt.month);
        dt.year = bcd_to_bin(dt.year);
    }

    apply_timezone(&dt);

    return dt;
}

const char *os_name(void)
{
    return OS_NAME;
}

const char *kernel_version(void)
{
    return KERNEL_VERSION;
}