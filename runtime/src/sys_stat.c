#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

#define SYS_FS_MAKE_DIR 11

static inline int fs_make_dir(const char *name)
{
    unsigned int ret;
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[n], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_MAKE_DIR), [n] "r"(name)
        : "eax", "ebx", "memory");
    return (int)ret;
}

int mkdir(const char *path, mode_t mode)
{
    (void)mode;
    int ret = fs_make_dir(path);
    return (ret == 0) ? 0 : -1;
}