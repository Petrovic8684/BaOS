#ifndef BAOS_INTERNAL_FS_HELPERS_H
#define BAOS_INTERNAL_FS_HELPERS_H

#include <errno.h>

int map_fs_error(int fs_error);

unsigned int fs_where_len(void);
char *fs_where(void);
int fs_change_dir(const char *name);
int fs_make_dir(const char *name);
int fs_delete_dir(const char *name);
unsigned int fs_list_dir_len(void);
char *fs_list_dir(void);
int fs_make_file(const char *name);
int fs_delete_file(const char *name);
int fs_write_file(const char *name, const unsigned char *data, unsigned int size);
int fs_read_file(const char *name, unsigned char *out_buf, unsigned int buf_size, unsigned int *out_size);
int fs_read_file_size(const char *name);

#endif
