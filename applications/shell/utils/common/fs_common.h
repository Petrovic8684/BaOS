#ifndef FS_COMMON_H
#define FS_COMMON_H

#include <stddef.h>
#include <stdbool.h>

int normalize_path(const char *input, char *out, size_t out_sz);
int join_paths(char *out, size_t out_sz, const char *base, const char *name);
int mkdirs_recursive(const char *path);
int copy_file_internal(const char *src, const char *dst);
int copy_dir_recursive(const char *src, const char *dst);
int remove_dir_recursive(const char *path);
bool is_subpath(const char *parent, const char *child);
void path_parent(const char *path, char *out, size_t out_sz);
const char *path_basename(const char *path);
int get_normalized_cwd(char *out, size_t out_sz);
const char *where(void);
const char *get_current_dir_name(void);

#endif
