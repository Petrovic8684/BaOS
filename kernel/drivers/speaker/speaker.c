#include "speaker.h"
#include "../../helpers/ports/ports.h"
#include "../display/display.h"
#include "../pit/pit.h"

#define PIT_CH2_PORT 0x42
#define PIT_CMD_PORT 0x43
#define PIT_CMD_SQUARE_WAVE_CH2 0xB6
#define SPEAKER_PORT 0x61

static unsigned char speaker_saved_port = 0;
static int speaker_inited = 0;

void speaker_init(void)
{
    if (speaker_inited)
        return;

    write("Initializing PC speaker...\n");

    speaker_saved_port = inb(SPEAKER_PORT);
    speaker_inited = 1;

    write("\033[32mPC speaker initialized.\033[0m\n\n");
}

void speaker_start(unsigned int hz)
{
    unsigned int divisor;
    unsigned char low, high;
    unsigned char p;

    if (!speaker_inited)
        speaker_init();

    if (hz == 0)
    {
        speaker_stop();
        return;
    }

    if (hz < 20)
        hz = 20;
    else if (hz > 20000)
        hz = 20000;

    if (hz > (PIT_FREQ_HZ / 1u))
        divisor = 1;
    else
        divisor = (PIT_FREQ_HZ + hz / 2u) / hz;

    if (divisor == 0)
        divisor = 1;
    if (divisor > 0xFFFFu)
        divisor = 0xFFFFu;

    low = (unsigned char)(divisor & 0xFFu);
    high = (unsigned char)((divisor >> 8) & 0xFFu);

    outb(PIT_CMD_PORT, PIT_CMD_SQUARE_WAVE_CH2);
    outb(PIT_CH2_PORT, low);
    outb(PIT_CH2_PORT, high);

    p = inb(SPEAKER_PORT);
    p |= 0x03;
    outb(SPEAKER_PORT, p);
}

void speaker_stop(void)
{
    unsigned char p = inb(SPEAKER_PORT);
    p &= ~0x03;
    outb(SPEAKER_PORT, p);
}

void speaker_beep(unsigned int hz, unsigned int ms)
{
    if (!speaker_inited)
        speaker_init();

    if (ms == 0)
        return;

    if (hz < 20)
        hz = 20;
    else if (hz > 20000)
        hz = 20000;

    speaker_start(hz);
    pit_sleep(ms);
    speaker_stop();
}
