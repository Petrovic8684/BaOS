#ifndef WRAPPERS_H
#define WRAPPERS_H

#include "../../kernel/drivers/display/display.h"
#include "../../kernel/drivers/keyboard/keyboard.h"
#include "../../kernel/drivers/rtc/rtc.h"
#include "../../kernel/fs/fs.h"
#include "../../kernel/helpers/string/string.h"
#include "../../kernel/system/system.h"

void wrapper_clear(void);
void wrapper_list_dir(void);
void wrapper_make_dir(const char *name);
void wrapper_make_file(const char *name);
void wrapper_echo(const char *arg1, const char *arg2);
void wrapper_print_date(void);
void wrapper_help(void);
void wrapper_os_name(void);
void wrapper_kernel_version(void);
void wrapper_change_dir(const char *name);
void wrapper_where(void);
void wrapper_delete_dir(const char *name);
void wrapper_delete_file(const char *name);
void wrapper_write_file(const char *name, const char *text);
void wrapper_read_file(const char *name);
void wrapper_info(const char *name);
void wrapper_shutdown(void);

#endif
