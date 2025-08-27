#ifndef DISPLAY_H
#define DISPLAY_H

#include "../../helpers/ports/ports.h"

void clear(void);
void write(const char *str);
void write_colored(const char *str, unsigned char color);
void write_char(char ch);

void scroll(void);
void draw_char_at(int r, int c, char ch);
void write_hex(unsigned int val);
void write_dec(unsigned int v);
void update_cursor(int new_row, int new_col);
int get_cursor_row(void);
int get_cursor_col(void);

extern int row;
extern int col;

#endif
