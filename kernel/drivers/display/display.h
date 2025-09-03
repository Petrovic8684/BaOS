#ifndef DISPLAY_H
#define DISPLAY_H

void write(const char *str);
void write_char(char ch);
void write_hex(unsigned int val);
void write_dec(unsigned int v);
void clear(void);

void write_dec_colored(unsigned int v, const char *color);
void write_hex_colored(unsigned int val, const char *color);
void write_char_colored(char ch, const char *color);

unsigned int get_cursor_row(void);
unsigned int get_cursor_col(void);

#endif
