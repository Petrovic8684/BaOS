#include <dirent.h>
#include "internal/fs_helpers.h"
#include "dirent/dirent_internal.h"
#include <errno.h>

int closedir(DIR *dirp)
{
    if (!dirp)
    {
        errno = EINVAL;
        return -1;
    }

    struct DIR *d = (struct DIR *)dirp;
    if (!d->in_use)
        return -1;

    d->in_use = 0;
    d->count = 0;
    d->pos = 0;
    d->dent.d_name[0] = '\0';
    d->dent.d_ino = 0;
    for (unsigned int i = 0; i < DIRENT_MAX_ENTRIES; i++)
        d->types[i] = DT_UNKNOWN;

    return 0;
}
