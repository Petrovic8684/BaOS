#include "drivers.h"
#include "./serial/serial.h"
#include "./keyboard/keyboard.h"
#include "./rtc/rtc.h"
#include "./pit/pit.h"
#include "./speaker/speaker.h"
#include "./mouse/mouse.h"

void drivers_init(int delay)
{
    serial_init();
    pit_init(100);
    pit_sleep(delay);
    keyboard_init();
    pit_sleep(delay);
    rtc_init();
    pit_sleep(delay);
    speaker_init();
    pit_sleep(delay);
    mouse_init();
    pit_sleep(delay);
}