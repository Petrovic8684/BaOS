#ifndef BAOS_STDIO_TTY_INTERNAL_H
#define BAOS_STDIO_TTY_INTERNAL_H

#include <stdio.h>

void send_move_syscall(int r, int c);
void clamp_cursor(int *row, int *col);
void scroll_up(int n);
void tty_wrap_char(unsigned char ch);
void tty_parse_after_write(const char *s);
void update_cursor(int new_row, int new_col);
void redraw_buffer(const char *buf, int len, int start_row, int start_col);
void ensure_space_and_scroll(int *start_row, int start_col, int cur_len, int old_len);
void tty_set_position(int row, int col);

#endif
