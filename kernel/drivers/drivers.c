#include "drivers.h"
#include "./keyboard/keyboard.h"
#include "./rtc/rtc.h"

void drivers_init(void)
{
    keyboard_init();
    rtc_init();
}