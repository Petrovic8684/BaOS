#include "pit.h"
#include "../../helpers/ports/ports.h"
#include "../../drivers/display/display.h"

#define PIT_FREQ_HZ 1193182u
#define PIT_CMD_PORT 0x43
#define PIT_CH0_PORT 0x40
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIT_CMD_SQUARE_WAVE 0x36

static volatile unsigned long long pit_ticks = 0;
static volatile unsigned long long pit_ms = 0;
static unsigned int pit_hz = 100;
static unsigned int ms_per_tick_q = 0;
static unsigned int ms_per_tick_r = 0;
static volatile unsigned int pit_ms_rem = 0;

static inline unsigned int save_and_cli(void)
{
    unsigned int flags;
    __asm__ volatile(
        "pushf\n\t"
        "pop %0\n\t"
        "cli"
        : "=r"(flags)
        :
        : "memory");
    return flags;
}

static inline void restore_flags(unsigned int flags)
{
    __asm__ volatile(
        "push %0\n\t"
        "popf"
        :
        : "r"(flags)
        : "memory", "cc");
}

void pit_irq_handler(int irq)
{
    (void)irq;
    pit_ticks++;
    pit_ms += ms_per_tick_q;
    pit_ms_rem += ms_per_tick_r;
    if (pit_ms_rem >= PIT_FREQ_HZ)
    {
        pit_ms++;
        pit_ms_rem -= PIT_FREQ_HZ;
    }
    outb(PIC1_COMMAND, 0x20);
}

void pit_init(unsigned int hz)
{
    write("Initializing PIT driver...\n");

    if (hz < 20 || hz >= 1000)
    {
        write("\033[32mFreq must be 20<hz<1000. Setting to hz=100...\n\033[0m");
        hz = 100;
    }

    pit_hz = hz;

    unsigned int divisor = (PIT_FREQ_HZ + hz / 2) / hz;
    if (divisor == 0)
        divisor = 1;
    if (divisor > 0xFFFFu)
        divisor = 0xFFFFu;

    unsigned int ms_per_tick_num = divisor * 1000u;
    ms_per_tick_q = ms_per_tick_num / PIT_FREQ_HZ;
    ms_per_tick_r = ms_per_tick_num % PIT_FREQ_HZ;

    pit_ticks = 0;
    pit_ms = 0;
    pit_ms_rem = 0;

    outb(PIT_CMD_PORT, PIT_CMD_SQUARE_WAVE);
    outb(PIT_CH0_PORT, (unsigned char)(divisor & 0xFF));
    outb(PIT_CH0_PORT, (unsigned char)((divisor >> 8) & 0xFF));

    unsigned char mask = inb(PIC1_DATA);
    mask &= ~(1 << 0);
    outb(PIC1_DATA, mask);

    write("\033[32mPIT driver initialized.\033[0m\n\n");
}

unsigned long long pit_get_ticks(void)
{
    unsigned long long t;
    unsigned int flags = save_and_cli();
    t = pit_ticks;
    restore_flags(flags);
    return t;
}

unsigned long long pit_get_ms(void)
{
    unsigned long long t;
    unsigned int flags = save_and_cli();
    t = pit_ms;
    restore_flags(flags);
    return t;
}

void pit_sleep(unsigned int ms)
{
    if (ms == 0)
        return;

    unsigned long product;
    if (pit_hz == 0)
        pit_hz = 100;
    if (ms > 0xFFFFFFFFu / pit_hz)
        product = 0xFFFFFFFFu;
    else
        product = ms * pit_hz;

    unsigned long ticks_needed = (product + 999u) / 1000u;
    if (ticks_needed == 0)
        ticks_needed = 1;

    unsigned long long end = pit_get_ticks() + (unsigned long long)ticks_needed;
    while (pit_get_ticks() < end)
        __asm__ volatile("hlt");
}
