#ifndef HISTORY_H
#define HISTORY_H

#include "../../kernel/drivers/keyboard/keyboard.h"
#include "../../kernel/drivers/display/display.h"
#include "../../kernel/helpers/string/string.h"

#define HISTORY_SIZE 10
#define MAX_CMD_LEN 80

void history_init(void);
void history_add(const char *cmd);
const char *history_prev(void);
const char *history_next(void);
void history_reset_index(void);
void history_read(char *buffer, int max_len, int prompt_len);

#endif
