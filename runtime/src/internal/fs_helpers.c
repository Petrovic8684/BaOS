#include "internal/fs_helpers.h"
#include "internal/syscalls.h"
#include <stdlib.h>
#include <string.h>


unsigned int fs_where_len(void)
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


char *fs_where(void)
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


int fs_change_dir(const char *name)
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


int fs_make_dir(const char *name)
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


int fs_delete_dir(const char *name)
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


unsigned int fs_list_dir_len(void)
{
    unsigned int len;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl $0, %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%eax, %[res]"
        : [res] "=r"(len)
        : [num] "i"(SYS_FS_LIST_DIR)
        : "eax", "ebx", "memory");

    return len;
}


char *fs_list_dir(void)
{
    unsigned int len = fs_list_dir_len();
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
        : [num] "i"(SYS_FS_LIST_DIR),
          [ptr] "r"(buf)
        : "eax", "ebx", "memory");

    if (ret < 0)
    {
        map_fs_error(ret);
        free(buf);
        return NULL;
    }

    return buf;
}


int fs_make_file(const char *name)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[n], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_MAKE_FILE), [n] "r"(name)
        : "eax", "ebx", "memory");

    if ((int)ret < 0)
        return map_fs_error((int)ret);

    return (int)ret;
}


int fs_delete_file(const char *name)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[n], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_DELETE_FILE), [n] "r"(name)
        : "eax", "ebx", "memory");

    if ((int)ret < 0)
        return map_fs_error((int)ret);

    return (int)ret;
}


int fs_write_file(const char *name, const unsigned char *data, unsigned int size)
{
    struct
    {
        const char *name;
        const unsigned char *data;
        unsigned int size;
    } args = {name, data, size};

    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[a], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_WRITE_FILE), [a] "r"(&args)
        : "eax", "ebx", "memory");

    if ((int)ret < 0)
        return map_fs_error((int)ret);

    return (int)ret;
}


int fs_read_file(const char *name, unsigned char *out_buf, unsigned int buf_size, unsigned int *out_size)
{
    struct
    {
        const char *name;
        unsigned char *out_buf;
        unsigned int buf_size;
        unsigned int *out_size;
    } args = {name, out_buf, buf_size, out_size};

    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[a], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_READ_FILE), [a] "r"(&args)
        : "eax", "ebx", "memory");

    if ((int)ret < 0)
        return map_fs_error((int)ret);

    return (int)ret;
}


int fs_read_file_size(const char *name)
{
    unsigned int out = 0;
    int rc = fs_read_file(name, NULL, 0, &out);
    if (rc < 0)
        return -1;
    return (int)out;
}
