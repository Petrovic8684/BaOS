#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int stat(const char *path, struct stat *buf)
{
    if (!path || !buf)
    {
        errno = EINVAL;
        return -1;
    }

    buf->st_mode = 0;
    buf->st_size = 0;

    DIR *d = opendir(path);
    if (d)
    {
        buf->st_mode = S_IFDIR;
        buf->st_size = 0;
        closedir(d);
        return 0;
    }

    FILE *f = fopen(path, "rb");
    if (f)
    {
        if (fseek(f, 0, SEEK_END) == 0)
        {
            long size = ftell(f);
            if (size >= 0)
                buf->st_size = (unsigned int)size;
        }
        fclose(f);
        buf->st_mode = S_IFREG;
        return 0;
    }

    errno = ENOENT;
    return -1;
}
