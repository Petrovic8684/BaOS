#include "rtc.h"
#include "../../helpers/bcd/bcd.h"
#include "../../helpers/ports/ports.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71
#define RTC_REG_A 0x0A
#define RTC_REG_B 0x0B
#define RTC_REG_C 0x0C

static volatile unsigned char sec, min, hour, day, mon, year;

static unsigned char cmos_read(unsigned char reg)
{
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

static void cmos_write(unsigned char reg, unsigned char val)
{
    outb(CMOS_ADDRESS, reg);
    outb(CMOS_DATA, val);
}

static void read_time_once(unsigned char *rsec, unsigned char *rmin, unsigned char *rhour, unsigned char *rday, unsigned char *rmon, unsigned char *ryear)
{
    *rsec = cmos_read(0x00);
    *rmin = cmos_read(0x02);
    *rhour = cmos_read(0x04);
    *rday = cmos_read(0x07);
    *rmon = cmos_read(0x08);
    *ryear = cmos_read(0x09);
}

void rtc_irq_handler(int irq)
{
    (void)irq;

    unsigned char regC = cmos_read(RTC_REG_C);
    if (!(regC & 0x10))
    {
        outb(0xA0, 0x20);
        outb(0x20, 0x20);
        return;
    }

    unsigned char regB = cmos_read(RTC_REG_B);

    unsigned char s1, m1, h1, d1, mo1, y1;
    unsigned char s2, m2, h2, d2, mo2, y2;

    do
    {
        read_time_once(&s1, &m1, &h1, &d1, &mo1, &y1);
        read_time_once(&s2, &m2, &h2, &d2, &mo2, &y2);
    } while (s1 != s2 || m1 != m2 || h1 != h2 || d1 != d2 || mo1 != mo2 || y1 != y2);

    if (!(regB & 0x04))
    {
        sec = bcd_to_bin(s1);
        min = bcd_to_bin(m1);
        hour = bcd_to_bin(h1);
        day = bcd_to_bin(d1);
        mon = bcd_to_bin(mo1);
        year = bcd_to_bin(y1);
    }
    else
    {
        sec = s1;
        min = m1;
        hour = h1;
        day = d1;
        mon = mo1;
        year = y1;
    }

    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

void rtc_init(void)
{
    unsigned char prev = cmos_read(RTC_REG_B);
    cmos_write(RTC_REG_B, prev | 0x10);

    unsigned char prevA = cmos_read(RTC_REG_A);
    cmos_write(RTC_REG_A, (prevA & 0xF0) | 0x06);
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
    unsigned char s, m, h, d, mo, y;
    unsigned long flags;

    __asm__ volatile("pushf\n\t"
                     "pop %0\n\t"
                     "cli"
                     : "=r"(flags)::"memory");

    s = sec;
    m = min;
    h = hour;
    d = day;
    mo = mon;
    y = year;

    __asm__ volatile("push %0\n\t"
                     "popf" ::"r"(flags) : "memory");

    int yr = (y < 100) ? (2000 + y) : y;
    long days = days_from_civil(yr, mo, d);
    long seconds = days * 86400 + h * 3600 + m * 60 + s;
    return (unsigned int)seconds;
}
