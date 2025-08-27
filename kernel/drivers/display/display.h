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
void write_hex(unsigned int val);
void write_dec(unsigned int v);
void redraw_buffer(char *buffer, int len, int prompt_len);
void redraw_and_clear(const char *buffer, int len, int prompt_len, int prev_len);

extern int row;
extern int col;

#endif
