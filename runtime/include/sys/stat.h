#ifndef SYS_STAT_H
#define SYS_STAT_H

typedef unsigned int mode_t;

#define S_IFDIR 0x4000
#define S_IFREG 0x8000

#define S_ISDIR(m) (((m) & 0xF000) == S_IFDIR)
#define S_ISFILE(m) (((m) & 0xF000) == S_IFREG)

struct stat
{
    mode_t st_mode;
    unsigned int st_size;
};

int mkdir(const char *path, mode_t mode);
int stat(const char *path, struct stat *buf);

#endif
