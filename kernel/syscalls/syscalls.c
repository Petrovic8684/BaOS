#include "../drivers/display/display.h"
#include "../drivers/keyboard/keyboard.h"
#include "../drivers/rtc/rtc.h"
#include "../fs/fs.h"
#include "../loader/loader.h"
#include "../system/system.h"
#include "../helpers/string/string.h"

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

extern unsigned int loader_return_eip;
extern unsigned int loader_saved_esp;
extern unsigned int loader_saved_ebp;

__attribute__((naked)) void return_to_loader(void)
{
    asm volatile(".intel_syntax noprefix\n\t"
                 "mov eax, dword ptr [loader_return_eip]\n\t"
                 "test eax, eax\n\t"
                 "jz 1f\n\t"
                 "mov esp, dword ptr [loader_saved_esp]\n\t"
                 "mov ebp, dword ptr [loader_saved_ebp]\n\t"
                 "jmp eax\n\t"
                 "1:\n\t"
                 "cli\n\t"
                 "hlt\n\t"
                 ".att_syntax\n\t");
}

void copy_from_user(char *kernel_buf, const char *user_buf, unsigned int max_len)
{
    unsigned int i = 0;
    for (; i < max_len - 1 && user_buf[i] != '\0'; i++)
        kernel_buf[i] = user_buf[i];

    kernel_buf[i] = '\0';
}

void copy_to_user(char *user_buf, const char *kernel_buf, unsigned int max_len)
{
    unsigned int i;
    for (i = 0; i < max_len - 1 && kernel_buf[i] != '\0'; i++)
        user_buf[i] = kernel_buf[i];
    user_buf[i] = '\0';
}

unsigned int handle_syscall(unsigned int num, unsigned int arg)
{
    switch (num)
    {
    case SYS_EXIT:
        return_to_loader();
        return 0;

    case SYS_WRITE:
    {
        char buffer[USER_BUFFER_SIZE];
        copy_from_user(buffer, (const char *)arg, sizeof(buffer));
        write(buffer);
        return 0;
    }

    case SYS_CLEAR:
        clear();
        return 0;

    case SYS_SCROLL:
        scroll();
        return 0;

    case SYS_GET_CURSOR_ROW:
        return (unsigned int)get_cursor_row();

    case SYS_GET_CURSOR_COL:
        return (unsigned int)get_cursor_col();

    case SYS_OS_NAME:
    {
        char *user_buf = (char *)arg;
        const char *name = os_name();
        copy_to_user(user_buf, name, USER_BUFFER_SIZE);
        return 0;
    }

    case SYS_KERNEL_VERSION:
    {
        char *user_buf = (char *)arg;
        const char *ver = kernel_version();
        copy_to_user(user_buf, ver, USER_BUFFER_SIZE);
        return 0;
    }

    case SYS_WRITE_COLORED:
    {
        struct
        {
            const char *str;
            unsigned char color;
        } *args = (void *)arg;
        char buffer[USER_BUFFER_SIZE];
        copy_from_user(buffer, args->str, sizeof(buffer));
        write_colored(buffer, args->color);
        return 0;
    }

    case SYS_DRAW_CHAR_AT:
    {
        struct
        {
            int r;
            int c;
            char ch;
        } *args = (void *)arg;
        draw_char_at(args->r, args->c, args->ch);
        return 0;
    }

    case SYS_READ:
    {
        char ch = read();
        return (unsigned int)ch;
    }

    case SYS_FS_WHERE:
        fs_where();
        return 0;

    case SYS_FS_GET_CURRENT_DIR:
    {
        char *user_buf = (char *)arg;
        const char *name = fs_get_current_dir_name();
        copy_to_user(user_buf, name, USER_BUFFER_SIZE);
        return 0;
    }

    case SYS_FS_MAKE_DIR:
        return fs_make_dir((const char *)arg);

    case SYS_FS_MAKE_FILE:
        return fs_make_file((const char *)arg);

    case SYS_FS_LIST_DIR:
        fs_list_dir();
        return 0;

    case SYS_FS_CHANGE_DIR:
        return fs_change_dir((const char *)arg);

    case SYS_FS_DELETE_DIR:
        return fs_delete_dir((const char *)arg);

    case SYS_FS_DELETE_FILE:
        return fs_delete_file((const char *)arg);

    case SYS_FS_WRITE_FILE:
    {
        struct
        {
            const char *name;
            const char *text;
        } *args = (void *)arg;
        return fs_write_file(args->name, args->text);
    }

    case SYS_FS_READ_FILE:
        return fs_read_file((const char *)arg);

    case SYS_FS_INFO:
        fs_info((const char *)arg);
        return 0;

    case SYS_RTC_NOW:
    {
        DateTime dt = rtc_now();
        char buf[5];

        itoa(dt.day, buf);
        write(buf);
        write("-");
        itoa(dt.month, buf);
        write(buf);
        write("-");
        itoa(dt.year, buf);
        write(buf);
        write(" ");
        itoa(dt.hour, buf);
        write(buf);
        write(":");
        itoa(dt.minute, buf);
        write(buf);
        write(":");
        itoa(dt.second, buf);
        write(buf);
        write("\n");

        return 0;
    }

    case SYS_POWER_OFF:
        loader_post_return_callback = power_off;

        return_to_loader();
        return 0;

    case SYS_LOAD_USER_PROGRAM:
    {
        const char *name = (const char *)arg;
        load_user_program(name);
        return 0;
    }

    case SYS_UPDATE_CURSOR:
    {
        struct
        {
            int row;
            int col;
        } *args = (void *)arg;

        update_cursor(args->row, args->col);
        return 0;
    }

    default:
        write_colored("Unknown syscall.\n", 0x04);
        return 0;
    }
}

__attribute__((naked)) void syscall_interrupt_handler()
{
    asm volatile(".intel_syntax noprefix\n\t"
                 "cli\n\t"
                 "push ebp\n\t"
                 "mov ebp, esp\n\t"
                 "push ebx\n\t"
                 "push eax\n\t"
                 "call handle_syscall\n\t"
                 "add esp, 8\n\t"
                 "mov ebx, eax\n\t"
                 "pop ebp\n\t"
                 "sti\n\t"
                 "iret\n\t"
                 ".att_syntax\n\t");
}