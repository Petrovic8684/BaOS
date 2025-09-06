#include "../drivers/display/display.h"
#include "../drivers/keyboard/keyboard.h"
#include "../drivers/rtc/rtc.h"
#include "../fs/fs.h"
#include "../helpers/string/string.h"
#include "../loader/loader.h"
#include "../info/info.h"
#include "../system/acpi/acpi.h"

#define SYS_EXIT 0
#define SYS_WRITE 1
#define SYS_READ 3
#define SYS_POWER_OFF 4
#define SYS_RTC_NOW 5
#define SYS_OS_NAME 6
#define SYS_KERNEL_VERSION 7
#define SYS_FS_WHERE 8
#define SYS_FS_LIST_DIR 9
#define SYS_FS_CHANGE_DIR 10
#define SYS_FS_MAKE_DIR 11
#define SYS_FS_DELETE_DIR 12
#define SYS_FS_MAKE_FILE 13
#define SYS_FS_DELETE_FILE 14
#define SYS_FS_WRITE_FILE 15
#define SYS_FS_READ_FILE 16
#define SYS_LOAD_USER_PROGRAM 17
#define SYS_GET_CURSOR_ROW 18
#define SYS_GET_CURSOR_COL 19
#define SYS_FS_WRITE_FILE_BIN 20

#define USER_BUFFER_SIZE 1024
#define MAX_ARGC 64

extern unsigned int loader_return_eip;
extern unsigned int loader_saved_esp;
extern unsigned int loader_saved_ebp;

extern void (*loader_post_return_callback)(void);

extern const char *next_prog_name;
extern const char **next_prog_argv;

static unsigned int last_return = 0;
static unsigned int should_report_return = 0;

static char saved_prog_argv_storage[MAX_ARGC][USER_BUFFER_SIZE];
static const char *saved_prog_argv_ptrs[MAX_ARGC + 1];

__attribute__((naked)) static void return_to_loader(void)
{
    asm volatile(".intel_syntax noprefix\n\t"
                 "mov ax, 0x10\n\t"
                 "mov ds, ax\n\t"
                 "mov es, ax\n\t"
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

static void copy_from_user(char *kernel_buf, const char *user_buf, unsigned int max_len)
{
    unsigned int i = 0;
    for (; i < max_len - 1 && user_buf[i] != '\0'; i++)
        kernel_buf[i] = user_buf[i];

    kernel_buf[i] = '\0';
}

static void copy_to_user(char *user_buf, const char *kernel_buf, unsigned int max_len)
{
    unsigned int i;
    for (i = 0; i < max_len - 1 && kernel_buf[i] != '\0'; i++)
        user_buf[i] = kernel_buf[i];
    user_buf[i] = '\0';
}

static void copy_bytes_to_user(char *user_buf, const char *kernel_buf, unsigned int len)
{
    unsigned int i;
    for (i = 0; i < len; i++)
        user_buf[i] = kernel_buf[i];
}

static void copy_bytes_from_user(unsigned char *kernel_buf, const unsigned char *user_buf, unsigned int len)
{
    unsigned int i;
    for (i = 0; i < len; i++)
        kernel_buf[i] = user_buf[i];
}

static unsigned int handle_syscall(unsigned int num, unsigned int arg)
{
    char kernel_buf[USER_BUFFER_SIZE];
    char *user_buf;

    switch (num)
    {
    case SYS_EXIT:
        last_return = arg;

        if (should_report_return)
        {
            write("\033[1;33mUser program returned: \033[0m");
            write_dec(last_return);
            write("\033[0m\n");

            should_report_return = 0;
            last_return = 0;
        }

        return_to_loader();
        return 0;

    case SYS_WRITE:
    {
        copy_from_user(kernel_buf, (const char *)arg, sizeof(kernel_buf));
        write(kernel_buf);
        return 0;
    }

    case SYS_READ:
    {
        char ch = read();
        return (unsigned int)ch;
    }

    case SYS_POWER_OFF:
        loader_post_return_callback = power_off;
        return_to_loader();
        return 0;

    case SYS_RTC_NOW:
    {
        unsigned int now = rtc_now();
        return now;
    }

    case SYS_OS_NAME:
    {
        user_buf = (char *)arg;
        const char *name = os_name();
        copy_to_user(user_buf, name, USER_BUFFER_SIZE);
        return 0;
    }

    case SYS_KERNEL_VERSION:
    {
        user_buf = (char *)arg;
        const char *ver = kernel_version();
        copy_to_user(user_buf, ver, USER_BUFFER_SIZE);
        return 0;
    }

    case SYS_FS_WHERE:
        user_buf = (char *)arg;
        copy_to_user(user_buf, fs_where(), USER_BUFFER_SIZE);
        return 0;

    case SYS_FS_LIST_DIR:
        user_buf = (char *)arg;
        copy_to_user(user_buf, fs_list_dir(), USER_BUFFER_SIZE);
        return 0;

    case SYS_FS_CHANGE_DIR:
        return fs_change_dir((const char *)arg);

    case SYS_FS_MAKE_DIR:
        return fs_make_dir((const char *)arg);

    case SYS_FS_DELETE_DIR:
        return fs_delete_dir((const char *)arg);

    case SYS_FS_MAKE_FILE:
        return fs_make_file((const char *)arg);

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
    {
        struct
        {
            const char *name;
            unsigned char *out_buf;
            unsigned int buf_size;
            unsigned int *out_size;
        } *uargs = (void *)arg;

        if (!uargs || !uargs->name)
            return (unsigned int)FS_ERR_NO_NAME;

        if (!((uargs->out_buf && uargs->buf_size > 0) || (uargs->out_buf == ((void *)0) && uargs->out_size)))
            return (unsigned int)FS_ERR_NO_NAME;

        static unsigned char kbuf[USER_BUFFER_SIZE];
        unsigned int out_sz = 0;

        unsigned int to_request = (uargs->buf_size > USER_BUFFER_SIZE) ? USER_BUFFER_SIZE : uargs->buf_size;

        int r = fs_read_file(uargs->name, (uargs->out_buf ? kbuf : ((void *)0)), to_request, &out_sz);
        if (r != FS_OK)
            return (unsigned int)r;

        if (uargs->out_buf && out_sz > 0)
            copy_bytes_to_user((char *)uargs->out_buf, (const char *)kbuf, out_sz);

        if (uargs->out_size)
            copy_bytes_to_user((char *)uargs->out_size, (const char *)&out_sz, sizeof(unsigned int));

        return 0;
    }

    case SYS_LOAD_USER_PROGRAM:
    {
        const char **user_argv = (const char **)arg;

        if (!user_argv || user_argv[0] == ((void *)0))
        {
            write("\033[31mError: No program specified.\033[0m\n");
            return 0;
        }

        int kargc = 0;
        should_report_return = 0;

        for (int i = 0; i < MAX_ARGC && user_argv[i] != ((void *)0); ++i)
        {
            if (i > 0 && str_equal(user_argv[i], "-code") == 1)
            {
                should_report_return = 1;
                continue;
            }

            copy_from_user(saved_prog_argv_storage[kargc], user_argv[i], USER_BUFFER_SIZE);
            saved_prog_argv_ptrs[kargc] = saved_prog_argv_storage[kargc];
            kargc++;
        }

        saved_prog_argv_ptrs[kargc] = ((void *)0);

        if (kargc == 0)
        {
            write("\033[31mError: No program specified after removing flags.\033[0m\n");
            return 0;
        }

        next_prog_argv = saved_prog_argv_ptrs;
        next_prog_name = saved_prog_argv_ptrs[0];

        loader_post_return_callback = load_next_program;

        return_to_loader();
        return 0;
    }

    case SYS_GET_CURSOR_ROW:
    {
        unsigned int row = get_cursor_row();
        return row;
    }

    case SYS_GET_CURSOR_COL:
    {
        unsigned int col = get_cursor_col();
        return col;
    }

    case SYS_FS_WRITE_FILE_BIN:
    {
        struct
        {
            const char *name;
            const unsigned char *data;
            unsigned int size;
        } *uargs = (void *)arg;

        if (!uargs || !uargs->name)
            return (unsigned int)FS_ERR_NO_NAME;

        const char *name = uargs->name;
        unsigned int total = uargs->size;
        const unsigned char *user_data = uargs->data;

        fs_delete_file(name);
        int r = fs_make_file(name);
        if (r != FS_OK)
            return (unsigned int)r;

        unsigned int off = 0;
        unsigned char kbuf[USER_BUFFER_SIZE];

        while (off < total)
        {
            unsigned int chunk = (total - off > USER_BUFFER_SIZE) ? USER_BUFFER_SIZE : (total - off);

            copy_bytes_from_user(kbuf, user_data + off, chunk);

            int wr = fs_write_file_bin(name, kbuf, chunk);
            if (wr != FS_OK)
                return (unsigned int)wr;

            off += chunk;
        }

        return 0;
    }

    default:
        write("\033[31mError: Unknown syscall.\n\033[0m");
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