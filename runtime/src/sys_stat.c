#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define SYS_FS_MAKE_DIR 11

static inline int fs_make_dir(const char *name)
{
    int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[n], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_MAKE_DIR), [n] "r"(name)
        : "eax", "ebx", "memory");

    return map_fs_error(ret);
}

int mkdir(const char *path, mode_t mode)
{
    (void)mode;
    int ret = fs_make_dir(path);
    return (ret == 0) ? 0 : -1;
}

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