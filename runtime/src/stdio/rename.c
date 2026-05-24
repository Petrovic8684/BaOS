#include <stdio.h>
#include "internal/syscalls.h"
#include <errno.h>
#include "internal/fs_helpers.h"

int rename(const char *oldpath, const char *newpath)
{
    if (!oldpath || !newpath)
    {
        errno = EINVAL;
        return -1;
    }

    FILE *oldf = fopen(oldpath, "r");
    if (!oldf)
        return -1;

    FILE *newf = fopen(newpath, "w");
    if (!newf)
    {
        fclose(oldf);
        return -1;
    }

    int ch;
    int write_err = 0;
    while ((ch = fgetc(oldf)) != EOF)
        if (fputc(ch, newf) == EOF)
        {
            write_err = 1;
            break;
        }

    if (!write_err && fflush(newf) == EOF)
        write_err = 1;

    int cerr_new = fclose(newf);
    int cerr_old = fclose(oldf);

    if (write_err || cerr_new != 0 || cerr_old != 0)
    {
        remove(newpath);
        errno = EIO;
        return -1;
    }

    int del = fs_delete_file(oldpath);
    if (del != 0)
        return -1;

    return 0;
}
