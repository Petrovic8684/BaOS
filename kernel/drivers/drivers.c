#include "drivers.h"
#include "./keyboard/keyboard.h"
#include "./rtc/rtc.h"
#include "./pit/pit.h"

void drivers_init(int delay)
{
    pit_init(100);
    pit_sleep(delay);
    keyboard_init();
    pit_sleep(delay);
    rtc_init();
    pit_sleep(delay);
}