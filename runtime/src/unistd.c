#include <unistd.h>
#include <string.h>

#define SYS_FS_WHERE 8
#define SYS_FS_CHANGE_DIR 10
#define SYS_FS_DELETE_DIR 12

#define USER_BUFFER_SIZE 2048

static inline const char *fs_where(void)
{
    static char buffer[USER_BUFFER_SIZE];
    asm volatile("movl %[num], %%eax\n\t"
                 "movl %[buffer], %%ebx\n\t"
                 "movl %[size], %%ecx\n\t"
                 "int $0x80\n\t" : : [num] "i"(SYS_FS_WHERE), [buffer] "r"(buffer), [size] "r"(USER_BUFFER_SIZE) : "eax", "ebx", "ecx", "memory");
    return buffer;
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
    const char *p = fs_where();
    if (!p)
        return NULL;

    size_t len = strlen(p);
    if (len + 1 > size)
        return NULL;

    memcpy(buf, p, len + 1);
    return buf;
}

int rmdir(const char *path)
{
    int ret = fs_delete_dir(path);

    if (ret == 0)
        return 0;

    return -1;
}
