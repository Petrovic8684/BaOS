#include <dirent.h>
#include "internal/fs_helpers.h"
#include "dirent/dirent_internal.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

DIR *opendir(const char *name)
{
    int idx = find_free_stream();
    if (idx < 0)
    {
        errno = EMFILE;
        return NULL;
    }

    struct DIR *d = &dir_streams[idx];
    memset(d, 0, sizeof(*d));
    d->in_use = 1;

    char *orig_p = fs_where();
    if (!orig_p)
        orig_p = strdup("/");

    int changed = 0;
    if (name && name[0] != '\0' && !(name[0] == '.' && name[1] == '\0'))
    {
        int ch = fs_change_dir(name);
        if (ch != 0)
        {
            free(orig_p);
            d->in_use = 0;
            return NULL;
        }
        changed = 1;
    }

    char *listing = fs_list_dir();
    if (listing)
    {
        parse_list_into_dir(d, listing);
        free(listing);
    }

    if (changed)
        fs_change_dir(orig_p);

    free(orig_p);
    return (DIR *)d;
}
