#ifndef USER_SYSCALLS_H
#define USER_SYSCALLS_H

#define SYS_EXIT 0
#define SYS_WRITE 1
#define SYS_CLEAR 2
#define SYS_SCROLL 3
#define SYS_GET_CURSOR_ROW 4
#define SYS_GET_CURSOR_COL 5
#define SYS_OS_NAME 6
#define SYS_KERNEL_VERSION 7
#define SYS_WRITE_COLORED 8
#define SYS_DRAW_CHAR_AT 9
#define SYS_READ 10
#define SYS_FS_WHERE 11
#define SYS_FS_GET_CURRENT_DIR 12
#define SYS_FS_MAKE_DIR 13
#define SYS_FS_MAKE_FILE 14
#define SYS_FS_LIST_DIR 15
#define SYS_FS_CHANGE_DIR 16
#define SYS_FS_DELETE_DIR 17
#define SYS_FS_DELETE_FILE 18
#define SYS_FS_WRITE_FILE 19
#define SYS_FS_READ_FILE 20
#define SYS_FS_INFO 21
#define SYS_RTC_NOW 22
#define SYS_POWER_OFF 23
#define SYS_LOAD_USER_PROGRAM 24
#define SYS_UPDATE_CURSOR 25

#define USER_BUFFER_SIZE 1024

static inline void sys_exit(int code)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[c], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_EXIT), [c] "r"(code)
        : "eax", "ebx", "memory");
}

static inline void write(const char *str)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[s], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_WRITE), [s] "r"(str)
        : "eax", "ebx", "memory");
}

static inline void clear()
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_CLEAR)
        : "eax", "memory");
}

static inline void scroll(void)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_SCROLL)
        : "eax", "memory");
}

static inline int get_cursor_row(void)
{
    int row;
    asm volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(row)
        : [num] "i"(SYS_GET_CURSOR_ROW)
        : "eax", "ebx", "memory");
    return row;
}

static inline int get_cursor_col(void)
{
    int col;
    asm volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(col)
        : [num] "i"(SYS_GET_CURSOR_COL)
        : "eax", "ebx", "memory");
    return col;
}

static inline const char *os_name(void)
{
    static char buffer[USER_BUFFER_SIZE];
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[buffer], %%ebx\n\t"
        "movl %[size], %%ecx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_OS_NAME),
          [buffer] "r"(buffer),
          [size] "r"(USER_BUFFER_SIZE)
        : "eax", "ebx", "ecx", "memory");
    return buffer;
}

static inline const char *kernel_version(void)
{
    static char buffer[USER_BUFFER_SIZE];
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[buffer], %%ebx\n\t"
        "movl %[size], %%ecx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_KERNEL_VERSION),
          [buffer] "r"(buffer),
          [size] "r"(USER_BUFFER_SIZE)
        : "eax", "ebx", "ecx", "memory");
    return buffer;
}

static inline void write_colored(const char *str, unsigned char color)
{
    struct
    {
        const char *str;
        unsigned char color;
    } args = {str, color};

    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[a], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_WRITE_COLORED), [a] "r"(&args)
        : "eax", "ebx", "memory");
}

static inline void draw_char_at(int r, int c, char ch)
{
    struct
    {
        int r;
        int c;
        char ch;
    } args = {r, c, ch};

    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[a], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_DRAW_CHAR_AT), [a] "r"(&args)
        : "eax", "ebx", "memory");
}

static inline char read(void)
{
    unsigned int ret;
    asm volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_READ)
        : "eax", "ebx", "memory");
    return (char)(ret & 0xFF);
}

static inline void fs_where(void)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_FS_WHERE)
        : "eax", "memory");
}

static inline const char *fs_get_current_dir_name(void)
{
    static char buffer[USER_BUFFER_SIZE];
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[buffer], %%ebx\n\t"
        "movl %[size], %%ecx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_FS_GET_CURRENT_DIR),
          [buffer] "r"(buffer),
          [size] "r"(USER_BUFFER_SIZE)
        : "eax", "ebx", "ecx", "memory");
    return buffer;
}

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

static inline int fs_make_file(const char *name)
{
    unsigned int ret;
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[n], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_MAKE_FILE), [n] "r"(name)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static inline void fs_list_dir(void)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_FS_LIST_DIR)
        : "eax", "memory");
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

static inline int fs_delete_file(const char *name)
{
    unsigned int ret;
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[n], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_DELETE_FILE), [n] "r"(name)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static inline int fs_write_file(const char *name, const char *text)
{
    struct
    {
        const char *name;
        const char *text;
    } args = {name, text};
    unsigned int ret;
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[a], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_WRITE_FILE), [a] "r"(&args)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static inline int fs_read_file(const char *name)
{
    unsigned int ret;
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[n], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_FS_READ_FILE), [n] "r"(name)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static inline void fs_info(const char *name)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[n], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_FS_INFO), [n] "r"(name)
        : "eax", "ebx", "memory");
}

static inline void rtc_now(void)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_RTC_NOW)
        : "eax", "memory");
}

static inline void power_off(void)
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_POWER_OFF)
        : "eax", "memory");
}

static inline void load_user_program(const char *name) // ?
{
    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[n], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_LOAD_USER_PROGRAM), [n] "r"(name)
        : "eax", "ebx", "memory");
}

static inline void update_cursor(int row, int col)
{
    struct
    {
        int row;
        int col;
    } args = {row, col};

    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[a], %%ebx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_UPDATE_CURSOR), [a] "r"(&args)
        : "eax", "ebx", "memory");
}

#endif
