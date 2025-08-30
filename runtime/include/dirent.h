#ifndef _DIRENT_H_
#define _DIRENT_H_

#include <stddef.h>

#define DIRENT_NAME_MAX 16
#define DIRENT_MAX_ENTRIES 32
#define DIRENT_MAX_STREAMS 4

#define DT_UNKNOWN 0
#define DT_REG 1
#define DT_DIR 2

struct dirent
{
    unsigned long d_ino;
    char d_name[DIRENT_NAME_MAX + 1];
    unsigned char d_type;
};

typedef struct DIR DIR;

DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
void rewinddir(DIR *dirp);
long telldir(DIR *dirp);
void seekdir(DIR *dirp, long loc);

#endif
