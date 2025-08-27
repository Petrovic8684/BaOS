#ifndef WRAPPERS_H
#define WRAPPERS_H

#include <stdio.h>
#include <stdlib.h>

#define FS_OK 0
#define FS_ERR_NO_DRV -1
#define FS_ERR_IO -2
#define FS_ERR_EXISTS -3
#define FS_ERR_NO_SPACE -4
#define FS_ERR_NAME_LONG -5
#define FS_ERR_NOT_INIT -6
#define FS_ERR_NO_NAME -7
#define FS_ERR_NO_TEXT -8
#define FS_ERR_NOT_EXISTS -9

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
void wrapper_filling(const char *name);
void wrapper_calc(const char *expr);

#endif