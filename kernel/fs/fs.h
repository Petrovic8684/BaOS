#ifndef FS_H
#define FS_H

#include "../IO/IO.h"
#include "../string/string.h"

#define MAX_FILES 16
#define MAX_DIRS 8
#define MAX_NAME 16

typedef struct File
{
    char name[MAX_NAME];
    char content[512];
    int size;
} File;

typedef struct Directory
{
    char name[MAX_NAME];
    struct Directory *parent;
    struct Directory *subdirs[MAX_DIRS];
    File *files[MAX_FILES];
    int dir_count;
    int file_count;
} Directory;

void fs_init(void);
const char *fs_get_current_dir_name(void);
void fs_make_dir(const char *name);
void fs_delete_dir(const char *name);
void fs_delete_file(const char *name);
void fs_make_file(const char *name);
void fs_list_dir(void);
void fs_change_dir(const char *name);
void fs_print_path(Directory *dir);
void fs_where(void);
void fs_info(const char *name);
void fs_write_file(const char *name, const char *text);
void fs_read_file(const char *name);

#endif
