#ifndef DISPLAY_H
#define DISPLAY_H

#include "../../helpers/ports/ports.h"

void clear(void);
void write(const char *str);
void write_colored(const char *str, unsigned char color);
void write_char(char ch);

void update_cursor(void);
void scroll(void);
void draw_char_at(int r, int c, char ch);

extern int row;
extern int col;

#endif
