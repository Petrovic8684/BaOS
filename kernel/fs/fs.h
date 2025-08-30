#ifndef FS_H
#define FS_H

#define MAX_NAME 16
#define MAX_DIRS 64
#define MAX_FILES 128
#define MAX_DIRS_PER_DIR 8
#define MAX_FILES_PER_DIR 16

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

typedef struct FS_File
{
    char name[MAX_NAME];
    unsigned int size;
    unsigned int data_lba;
} FS_File;

typedef struct FS_Dir
{
    char name[MAX_NAME];
    unsigned int parent_lba;
    unsigned int dirs_lba[MAX_DIRS_PER_DIR];
    unsigned int files_lba[MAX_FILES_PER_DIR];
    unsigned char dir_count;
    unsigned char file_count;
    unsigned char padding[2];
} FS_Dir;

typedef struct
{
    unsigned int magic;
    unsigned int max_dirs;
    unsigned int max_files;
    unsigned int dir_table_start;
    unsigned int file_table_start;
    unsigned int data_start;
    unsigned int root_dir_lba;
    unsigned char dir_bitmap[MAX_DIRS];
    unsigned char file_bitmap[MAX_FILES];
    unsigned char padding[512 - (7 * 4) - MAX_DIRS - MAX_FILES];
} FS_SuperOnDisk;

int fs_is_initialized(void);
void fs_init(void);
int fs_make_dir(const char *name);
int fs_make_file(const char *name);
const char *fs_list_dir(void);
int fs_change_dir(const char *name);
const char *fs_where(void);
int fs_delete_dir(const char *name);
int fs_delete_file(const char *name);
int fs_write_file(const char *name, const char *text);
int fs_read_file(const char *name, unsigned char *out_buf, unsigned int buf_size, unsigned int *out_size);

#endif
