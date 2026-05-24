#include <dirent.h>
#include "internal/fs_helpers.h"
#include "dirent/dirent_internal.h"
#include <errno.h>

struct dirent *readdir(DIR *dirp)
{
    if (!dirp)
    {
        errno = EINVAL;
        return NULL;
    }

    struct DIR *d = (struct DIR *)dirp;
    if (!d->in_use)
        return NULL;

    if ((unsigned long)d->pos >= d->count)
        return NULL;

    d->dent.d_ino = (unsigned long)(d->pos + 1);

    unsigned int i = 0;
    for (; i < DIRENT_NAME_MAX && d->names[d->pos][i] != '\0'; i++)
        d->dent.d_name[i] = d->names[d->pos][i];
    d->dent.d_name[i] = '\0';

    d->dent.d_type = d->types[d->pos];

    d->pos++;
    return &d->dent;
}
