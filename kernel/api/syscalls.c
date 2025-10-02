#include "../drivers/display/display.h"
#include "../drivers/keyboard/keyboard.h"
#include "../drivers/speaker/speaker.h"
#include "../drivers/mouse/mouse.h"
#include "../drivers/rtc/rtc.h"
#include "../drivers/pit/pit.h"
#include "../fs/fs.h"
#include "../paging/paging.h"
#include "../paging/heap/heap.h"
#include "../helpers/string/string.h"
#include "../helpers/memory/memory.h"
#include "../loader/loader.h"
#include "../info/sys/sys.h"
#include "../system/acpi/acpi.h"

#define SYS_EXIT 0
#define SYS_WRITE 1
#define SYS_READ 3
#define SYS_POWER_OFF 4
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
#define SYS_LOAD_USER_PROGRAM 17
#define SYS_GET_CURSOR_ROW 18
#define SYS_GET_CURSOR_COL 19
#define SYS_REBOOT 20
#define SYS_SET_USER_PAGES 21
#define SYS_HEAP_INFO 22
#define SYS_SLEEP 23
#define SYS_UPTIME 24
#define SYS_BEEP 25
#define SYS_MOUSE_READ 30
#define SYS_MOUSE_PEEK 31
#define SYS_MOUSE_GETPOS 32
#define SYS_MOUSE_HAS_WHEEL 33

extern void (*loader_post_return_callback)(void);

static unsigned int last_return = 0;
static unsigned int should_report_return = 0;

static char saved_prog_argv_storage[MAX_ARGC][MAX_ARGV_LEN];
static const char *saved_prog_argv_ptrs[MAX_ARGC + 1];

static unsigned int handle_syscall(unsigned int num, unsigned int arg)
{
    switch (num)
    {
    case SYS_EXIT:
        last_return = arg;

        if (should_report_return)
        {
            write("\033[1;33mUser program returned: ");
            write_dec(last_return);
            write("\033[0m\n");

            should_report_return = 0;
            last_return = 0;
        }

        return_to_loader();
        return 0;

    case SYS_WRITE:
    {
        const char *text = (const char *)arg;
        write(text);
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

    case SYS_SYS_INFO:
    {
        struct utsname *buf = (struct utsname *)arg;
        mem_copy(buf, &uname_info, sizeof(struct utsname));

        return 0;
    }

    case SYS_FS_WHERE:
    {
        char *user_buf = (char *)arg;
        int err;
        char *path = fs_where(&err);
        if (!path)
            return -err;

        unsigned int len = str_count(path) + 1;
        if (!user_buf)
        {
            kfree(path);
            return len;
        }

        mem_copy(user_buf, path, len);
        kfree(path);
        return FS_OK;
    }

    case SYS_FS_LIST_DIR:
    {
        char *user_buf = (char *)arg;
        int err = 0;
        char *contents = fs_list_dir(&err);

        if (!contents)
            return -err;

        unsigned int len = str_count(contents) + 1;

        if (!user_buf)
        {
            kfree(contents);
            return len;
        }

        mem_copy(user_buf, contents, len);
        kfree(contents);
        return FS_OK;
    }

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
            const unsigned char *data;
            unsigned int size;
        } *uargs = (void *)arg;

        if (!uargs || !uargs->name)
            return (unsigned int)FS_ERR_NO_NAME;

        const char *name = uargs->name;

        fs_delete_file(name);
        int r = fs_make_file(name);
        if (r != FS_OK)
            return (unsigned int)r;

        if (uargs->data && uargs->size > 0)
        {
            r = fs_write_file(name, uargs->data, uargs->size);
            if (r != FS_OK)
                return (unsigned int)r;
        }

        return 0;
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

        unsigned int out_sz = 0;
        int r;

        if (uargs->out_buf)
            r = fs_read_file(uargs->name, uargs->out_buf, uargs->buf_size, &out_sz);
        else
            r = fs_read_file(uargs->name, ((void *)0), 0, &out_sz);

        if (r != FS_OK)
            return (unsigned int)r;

        if (uargs->out_size)
            mem_copy((char *)uargs->out_size, (const char *)&out_sz, sizeof(unsigned int));

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

            mem_copy(saved_prog_argv_storage[kargc], user_argv[i], str_count(user_argv[i]) + 1);
            saved_prog_argv_ptrs[kargc] = saved_prog_argv_storage[kargc];
            kargc++;
        }

        saved_prog_argv_ptrs[kargc] = ((void *)0);

        if (kargc == 0)
        {
            write("\033[31mError: No program specified after removing flags.\033[0m\n");
            return 0;
        }

        set_next_program(saved_prog_argv_ptrs);
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

    case SYS_REBOOT:
        loader_post_return_callback = reboot;
        return_to_loader();
        return 0;

    case SYS_SET_USER_PAGES:
    {
        if (arg == 0)
            return -14;

        struct
        {
            unsigned int virt_start;
            unsigned int size;
        } kargs;

        mem_copy((unsigned char *)&kargs, (const unsigned char *)arg, sizeof(kargs));
        int ret = set_user_pages(kargs.virt_start, kargs.size);

        return ret;
    }

    case SYS_HEAP_INFO:
    {
        struct heap_info info;
        get_heap_info(&info);
        mem_copy((void *)arg, &info, sizeof(info));
        return 0;
    }

    case SYS_SLEEP:
    {
        unsigned int ms = arg;
        if (ms == 0)
            return 0;

        __asm__ volatile("sti");
        pit_sleep(ms);
        __asm__ volatile("cli");

        return 0;
    }

    case SYS_UPTIME:
    {
        unsigned long long ms = pit_get_ms();

        unsigned long high = (unsigned long)(ms >> 32);
        unsigned long low = (unsigned long)(ms & 0xFFFFFFFF);

        unsigned long s = high * (4294967296UL / 1000);
        unsigned long rem = high * (4294967296UL % 1000);
        rem = rem + low;
        s += rem / 1000;

        return s;
    }

    case SYS_BEEP:
    {
        struct
        {
            unsigned int hz;
            unsigned int ms;
        } kargs;

        mem_copy(&kargs, (const void *)arg, sizeof(kargs));

        if (kargs.hz == 0)
            return 0;

        if (kargs.hz < 20)
            kargs.hz = 20;
        else if (kargs.hz > 20000)
            kargs.hz = 20000;

        __asm__ volatile("sti");
        speaker_beep(kargs.hz, kargs.ms);
        __asm__ volatile("cli");

        return 0;
    }

    case SYS_MOUSE_READ:
    {
        mouse_event_t *user_ev = (mouse_event_t *)arg;
        if (!user_ev)
            return 0;

        return mouse_read_event(user_ev);
    }

    case SYS_MOUSE_PEEK:
    {
        mouse_event_t *user_ev = (mouse_event_t *)arg;
        return mouse_peek_event(user_ev);
    }

    case SYS_MOUSE_GETPOS:
    {
        int *user_coords = (int *)arg;
        if (!user_coords)
            return 0;

        int x = 0, y = 0;
        mouse_get_position(&x, &y);

        mem_copy((void *)user_coords, (const void *)&x, sizeof(int));
        mem_copy((void *)(user_coords + 1), (const void *)&y, sizeof(int));

        return 0;
    }

    case SYS_MOUSE_HAS_WHEEL:
    {
        return mouse_has_wheel();
    }

    default:
        write("\033[31mError: Unknown syscall.\n\033[0m");
        return 0;
    }
}

__attribute__((naked)) void syscall_interrupt_handler()
{
    __asm__ volatile(".intel_syntax noprefix\n\t"
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