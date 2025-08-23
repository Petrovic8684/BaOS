#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../display/display.h"
#include "../../helpers/ports/ports.h"

void read(char *buffer, int max_len, int prompt_len);

#endif
