#ifndef DIRLIST_COMMON_H
#define DIRLIST_COMMON_H

#include <stdio.h>
#include <dirent.h>
#include <stddef.h>
#include <baos/vga.h>

typedef struct
{
    char name[DIRENT_NAME_MAX + 1];
    int is_dir;
    int row;
    int col;
    int len;
} dir_entry_t;

int dirlist_print_contents(const char *path, FILE *out);
int dirlist_collect_entries(const char *path, dir_entry_t *out, int max, int start_row);
int dirlist_scan_listing_entries(const char *base_path, dir_entry_t *out, int max);
int dirlist_resolve_path(const char *path, char *out, size_t outsz);
void dirlist_join_path(char *out, size_t outsz, const char *base, const char *name);
int dirlist_save_context(const char *path);
int dirlist_load_context(char *buf, size_t size);
void dirlist_clear_context(void);

#endif
