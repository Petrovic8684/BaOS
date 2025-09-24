#ifndef RTC_H
#define RTC_H

void rtc_irq_handler(int irq);

void rtc_init(void);
unsigned int rtc_now(void);

#endif
