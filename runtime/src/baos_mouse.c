#include <baos/mouse.h>

#define SYS_MOUSE_READ 30
#define SYS_MOUSE_PEEK 31
#define SYS_MOUSE_GETPOS 32
#define SYS_MOUSE_HAS_WHEEL 33

static inline int sys_mouse_read(mouse_event_t *ev)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[arg], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_MOUSE_READ), [arg] "r"(ev)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static inline int sys_mouse_peek(mouse_event_t *ev)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[arg], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_MOUSE_PEEK), [arg] "r"(ev)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static inline int sys_mouse_getpos(int *coords)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "movl %[arg], %%ebx\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_MOUSE_GETPOS), [arg] "r"(coords)
        : "eax", "ebx", "memory");
    return (int)ret;
}

static inline int sys_mouse_has_wheel(void)
{
    unsigned int ret;
    __asm__ volatile(
        "movl %[num], %%eax\n\t"
        "int $0x80\n\t"
        "movl %%ebx, %[res]\n\t"
        : [res] "=r"(ret)
        : [num] "i"(SYS_MOUSE_HAS_WHEEL)
        : "eax", "ebx", "memory");
    return (int)ret;
}

int mouse_read(mouse_event_t *ev)
{
    return sys_mouse_read(ev);
}

int mouse_peek(mouse_event_t *ev)
{
    return sys_mouse_peek(ev);
}

void mouse_getpos(int *x, int *y)
{
    int coords[2];
    sys_mouse_getpos(coords);
    if (x)
        *x = coords[0];
    if (y)
        *y = coords[1];
}

int mouse_has_wheel(void)
{
    return sys_mouse_has_wheel();
}
