#ifndef HISTORY_H
#define HISTORY_H

#include <string.h>

#define HISTORY_SIZE 10

void history_init(void);
void history_add(const char *cmd);
const char *history_prev(void);
const char *history_next(void);
void history_reset_index(void);

#endif