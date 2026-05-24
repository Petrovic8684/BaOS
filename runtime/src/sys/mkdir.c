#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "internal/fs_helpers.h"

int mkdir(const char *path, mode_t mode)
{
    (void)mode;
    int ret = fs_make_dir(path);
    return (ret == 0) ? 0 : -1;
}
