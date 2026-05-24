#ifndef BAOS_INTERNAL_SYSCALLS_H
#define BAOS_INTERNAL_SYSCALLS_H

#define SYS_EXIT 0
#define SYS_WRITE 1
#define SYS_READ 3
#define SYS_RTC_NOW 5
#define SYS_SYS_INFO 6
#define SYS_FS_WHERE 8
#define SYS_FS_LIST_DIR 9
#define SYS_FS_CHANGE_DIR 10
#define SYS_FS_MAKE_DIR 11
#define SYS_FS_DELETE_DIR 12
#define SYS_FS_MAKE_FILE 13
#define SYS_FS_DELETE_FILE 14
#define SYS_FS_WRITE_FILE 15
#define SYS_FS_READ_FILE 16
#define SYS_GET_CURSOR_ROW 18
#define SYS_GET_CURSOR_COL 19
#define SYS_SET_USER_PAGES 21
#define SYS_SLEEP 23
#define SYS_UPTIME 24

static inline void sys_write(const char *str)
{
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[s], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_WRITE), [s] "r"(str)
        : "eax", "ebx", "memory");
}

static inline unsigned char sys_read(void)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_READ)
        : "eax", "ebx", "memory");
    return (unsigned char)(ret & 0xFF);
}

static inline int sys_get_cursor_row(void)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_GET_CURSOR_ROW)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static inline int sys_get_cursor_col(void)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_GET_CURSOR_COL)
        : "eax", "ebx", "memory");
    return (int)ret;
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

#endif
