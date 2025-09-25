#ifndef PIT_H
#define PIT_H

void pit_irq_handler(int irq);

void pit_init(unsigned int hz);
unsigned long long pit_get_ticks(void);
unsigned long long pit_get_ms(void);
void pit_sleep(unsigned int ms);

#endif
