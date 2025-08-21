#ifndef IO_H
#define IO_H

#include "../ports/ports.h"

void clear(void);
void write(const char *str);
void write_colored(const char *str, unsigned char color);
void write_char(char ch);
void read(char *buffer, int max_len, int prompt_len);

#endif
