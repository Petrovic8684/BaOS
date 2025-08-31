#ifndef WRAPPERS_H
#define WRAPPERS_H

#define SYS_POWER_OFF 4
#define SYS_LOAD_USER_PROGRAM 17

void wrapper_clear(void);
void wrapper_list_dir(const char *path);
void wrapper_make_dir(const char *name);
void wrapper_make_file(const char *name);
void wrapper_echo(const char *arg1, const char *arg2);
void wrapper_print_date(void);
void wrapper_help(void);
void wrapper_os_name(void);
void wrapper_kernel_version(void);
void wrapper_where(void);
const char *wrapper_get_current_dir_name(void);
void wrapper_change_dir(const char *name);
void wrapper_delete_dir(const char *name);
void wrapper_delete_file(const char *name);
void wrapper_write_file(const char *name, const char *text);
void wrapper_read_file(const char *name);
void wrapper_shutdown(void);
void wrapper_run(const char *name, const char *args);
void wrapper_copy(const char *src, const char *dst);
void wrapper_move(const char *src, const char *dst);

#endif