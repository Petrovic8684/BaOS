#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../display/display.h"
#include "../../helpers/ports/ports.h"

#define ARR_UP 1
#define ARR_DOWN 2
#define ARR_LEFT 3
#define ARR_RIGHT 4
#define ARR_NONE 0

char read(void);

#endif
