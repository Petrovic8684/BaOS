#include <dirent.h>
#include "internal/fs_helpers.h"
#include "dirent/dirent_internal.h"
#include <errno.h>

void seekdir(DIR *dirp, long loc)
{
    if (!dirp)
    {
        errno = EINVAL;
        return;
    }
    struct DIR *d = (struct DIR *)dirp;
    if (!d->in_use)
        return;
    if (loc < 0)
        d->pos = 0;
    else if ((unsigned long)loc > d->count)
        d->pos = d->count;
    else
        d->pos = loc;
}
