#include <stdio.h>
#include "internal/syscalls.h"
#include "stdio/tty_internal.h"

void write(const char *str)
{
    if (!str)
        return;

    sys_write(str);
    tty_parse_after_write(str);

    int real_row = sys_get_cursor_row();
    int real_col = sys_get_cursor_col();
    tty_set_position(real_row, real_col);
}
