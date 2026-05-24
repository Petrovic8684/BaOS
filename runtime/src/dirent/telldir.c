#include <dirent.h>
#include "internal/fs_helpers.h"
#include "dirent/dirent_internal.h"
#include <errno.h>

long telldir(DIR *dirp)
{
    if (!dirp)
    {
        errno = EINVAL;
        return -1;
    }
    struct DIR *d = (struct DIR *)dirp;
    if (!d->in_use)
        return -1;
    return d->pos;
}
