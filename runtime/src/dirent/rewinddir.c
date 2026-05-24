#include <dirent.h>
#include "internal/fs_helpers.h"
#include "dirent/dirent_internal.h"
#include <errno.h>

void rewinddir(DIR *dirp)
{
    if (!dirp)
    {
        errno = EINVAL;
        return;
    }
    struct DIR *d = (struct DIR *)dirp;
    if (!d->in_use)
        return;
    d->pos = 0;
}
