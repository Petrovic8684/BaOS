#ifndef PIT_H
#define PIT_H

#define PIT_FREQ_HZ 1193182u

typedef void (*pit_tick_callback_t)(unsigned long long ticks);

void pit_irq_handler(int irq);
void pit_init(unsigned int hz);
unsigned long long pit_get_ticks(void);
unsigned long long pit_get_ms(void);
void pit_sleep(unsigned int ms);
void pit_register_tick_callback(pit_tick_callback_t cb);
unsigned int pit_get_hz(void);

#endif
