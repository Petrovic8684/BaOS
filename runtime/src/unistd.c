#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define SYS_FS_WHERE 8
#define SYS_FS_CHANGE_DIR 10
#define SYS_FS_DELETE_DIR 12

static unsigned int fs_where_len(void)
{
    unsigned int len;
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl $0, %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%eax, %[res]"
        : [res] "=r"(len)
        : [num] "i"(SYS_FS_WHERE)
        : "eax", "ebx", "memory");
    return len;
}

static char *fs_where(void)
{
    unsigned int len = fs_where_len();
    if (len == 0)
        return NULL;

    char *buf = malloc(len);
    if (!buf)
        return NULL;

    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[ptr], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_FS_WHERE), [ptr] "r"(buf)
        : "eax", "ebx", "memory");

    return buf;
}

static inline int fs_change_dir(const char *name)
{
    unsigned int ret;
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[n], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_CHANGE_DIR), [n] "r"(name)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static inline int fs_delete_dir(const char *name)
{
    unsigned int ret;
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[n], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_DELETE_DIR), [n] "r"(name)
        : "eax", "ebx", "memory");
    return (int)ret;
}

int chdir(const char *path)
{
    return fs_change_dir(path);
}

char *getcwd(char *buf, size_t size)
{
    char *p = fs_where();
    if (!p)
        return NULL;

    size_t len = strlen(p);

    if (buf == NULL)
    {
        buf = malloc(len + 1);
        if (!buf)
        {
            free(p);
            return NULL;
        }
    }
    else if (len + 1 > size)
    {
        free(p);
        return NULL;
    }

    memcpy(buf, p, len + 1);
    free(p);
    return buf;
}

int rmdir(const char *path)
{
    int ret = fs_delete_dir(path);

    if (ret == 0)
        return 0;

    return -1;
}
