#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define SYS_FS_WHERE 8
#define SYS_FS_CHANGE_DIR 10
#define SYS_FS_DELETE_DIR 12
#define SYS_SLEEP 23

static unsigned int fs_where_len(void)
{
    int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl $0, %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%eax, %[res]"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_WHERE)
        : "eax", "ebx", "memory");

    if (ret < 0)
    {
        errno = -ret;
        return 0;
    }

    return (unsigned int)ret;
}

static char *fs_where(void)
{
    unsigned int len = fs_where_len();
    if (len == 0)
        return NULL;

    char *buf = malloc(len);
    if (!buf)
    {
        errno = ENOMEM;
        return NULL;
    }

    int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[ptr], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%eax, %[res]"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_WHERE), [ptr] "r"(buf)
        : "eax", "ebx", "memory");

    if (ret < 0)
    {
        free(buf);
        errno = -ret;
        return NULL;
    }

    return buf;
}

static inline int fs_change_dir(const char *name)
{
    int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[n], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%eax, %[res]"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_CHANGE_DIR), [n] "r"(name)
        : "eax", "ebx", "memory");

    if (ret < 0)
        return map_fs_error(ret);

    return 0;
}

static inline int fs_delete_dir(const char *name)
{
    int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[n], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%eax, %[res]"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_DELETE_DIR), [n] "r"(name)
        : "eax", "ebx", "memory");

    if (ret < 0)
        return map_fs_error(ret);

    return 0;
}

static inline void sys_sleep(unsigned int ms)
{
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[arg], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_SLEEP), [arg] "r"(ms)
        : "eax", "ebx", "memory");
}

int chdir(const char *path)
{
    return fs_change_dir(path);
}

char *getcwd(char *buf, size_t size)
{
    char *path = fs_where();
    if (!path)
        return NULL;

    size_t len = strlen(path);

    char *retbuf = buf;
    if (!buf)
    {
        retbuf = malloc(len + 1);
        if (!retbuf)
        {
            free(path);
            errno = ENOMEM;
            return NULL;
        }
    }
    else if (len + 1 > size)
    {
        free(path);
        errno = ERANGE;
        return NULL;
    }

    memcpy(retbuf, path, len + 1);
    free(path);

    return retbuf;
}

int rmdir(const char *path)
{
    int ret = fs_delete_dir(path);

    if (ret == 0)
        return 0;

    return -1;
}

unsigned int sleep(unsigned int seconds)
{
    sys_sleep(seconds * 1000);
    return 0;
}

int usleep(useconds_t usec)
{
    unsigned int ms = (usec + 999) / 1000;
    sys_sleep(ms);
    return 0;
}