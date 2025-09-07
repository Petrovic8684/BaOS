#include "keyboard.h"
#include "../display/display.h"
#include "../../helpers/ports/ports.h"

#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define KB_BUF_SIZE 256

#define KEY_ARR_UP 1
#define KEY_ARR_DOWN 2
#define KEY_ARR_LEFT 3
#define KEY_ARR_RIGHT 4
#define KEY_ESCAPE 27
#define KEY_DELETE 127
#define KEY_SHIFT_BACKSPACE 128
#define KEY_CTRL_BACKSPACE 129
#define KEY_SHIFT_DELETE 130
#define KEY_CTRL_DELETE 131
#define KEY_CTRL_LEFT 132
#define KEY_CTRL_RIGHT 133
#define KEY_CTRL_UP 134
#define KEY_CTRL_DOWN 135
#define KEY_SHIFT_LEFT 136
#define KEY_SHIFT_RIGHT 137
#define KEY_SHIFT_UP 138
#define KEY_SHIFT_DOWN 139
#define KEY_CTRL_X 140
#define KEY_CTRL_C 141
#define KEY_CTRL_V 142

static volatile unsigned char kb_buf[KB_BUF_SIZE];
static volatile unsigned int kb_head = 0;
static volatile unsigned int kb_tail = 0;
static volatile unsigned int kb_count = 0;

static void kb_push(unsigned char ch)
{
    unsigned int next = (kb_head + 1) & (KB_BUF_SIZE - 1);
    if (next == kb_tail)
    {
        kb_tail = (kb_tail + 1) & (KB_BUF_SIZE - 1);
        kb_count--;
    }
    kb_buf[kb_head] = ch;
    kb_head = next;
    kb_count++;
}

static int kb_pop(unsigned char *out)
{
    if (kb_count == 0)
        return 0;
    *out = kb_buf[kb_tail];
    kb_tail = (kb_tail + 1) & (KB_BUF_SIZE - 1);
    kb_count--;
    return 1;
}

static const unsigned char normal[128] = {
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4', [0x06] = '5', [0x07] = '6', [0x08] = '7', [0x09] = '8', [0x0A] = '9', [0x0B] = '0', [0x0C] = '-', [0x0D] = '=', [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r', [0x14] = 't', [0x15] = 'y', [0x16] = 'u', [0x17] = 'i', [0x18] = 'o', [0x19] = 'p', [0x1A] = '[', [0x1B] = ']', [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f', [0x22] = 'g', [0x23] = 'h', [0x24] = 'j', [0x25] = 'k', [0x26] = 'l', [0x27] = ';', [0x28] = '\'', [0x2B] = '\\', [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v', [0x30] = 'b', [0x31] = 'n', [0x32] = 'm', [0x33] = ',', [0x34] = '.', [0x35] = '/', [0x39] = ' '};
static const unsigned char shift_map[128] = {
    [0x02] = '!', [0x03] = '@', [0x04] = '#', [0x05] = '$', [0x06] = '%', [0x07] = '^', [0x08] = '&', [0x09] = '*', [0x0A] = '(', [0x0B] = ')', [0x0C] = '_', [0x0D] = '+', [0x10] = 'Q', [0x11] = 'W', [0x12] = 'E', [0x13] = 'R', [0x14] = 'T', [0x15] = 'Y', [0x16] = 'U', [0x17] = 'I', [0x18] = 'O', [0x19] = 'P', [0x1A] = '{', [0x1B] = '}', [0x1E] = 'A', [0x1F] = 'S', [0x20] = 'D', [0x21] = 'F', [0x22] = 'G', [0x23] = 'H', [0x24] = 'J', [0x25] = 'K', [0x26] = 'L', [0x27] = ':', [0x28] = '"', [0x2B] = '|', [0x2C] = 'Z', [0x2D] = 'X', [0x2E] = 'C', [0x2F] = 'V', [0x30] = 'B', [0x31] = 'N', [0x32] = 'M', [0x33] = '<', [0x34] = '>', [0x35] = '?', [0x39] = ' '};

static unsigned char shift = 0;
static unsigned char extended = 0;
static unsigned char ctrl = 0;

void keyboard_interrupt_handler_c(void)
{
    unsigned char scancode = inb(0x60);

    if (scancode == 0xE0)
    {
        extended = 1;
        return;
    }

    if (scancode == 0x2A || scancode == 0x36)
    {
        shift = 1;
        extended = 0;
        return;
    }

    if (scancode == 0xAA || scancode == 0xB6)
    {
        shift = 0;
        extended = 0;
        return;
    }

    if (scancode == 0x1D)
    {
        ctrl = 1;
        extended = 0;
        return;
    }

    if (scancode & 0x80)
    {
        if (scancode == 0x9D)
        {
            ctrl = 0;
            extended = 0;
            return;
        }
        extended = 0;
        return;
    }

    if (extended)
    {
        unsigned char e_scancode = scancode;
        extended = 0;

        switch (e_scancode)
        {
        case 0x48:
            if (shift)
                kb_push(KEY_SHIFT_UP);
            else if (ctrl)
                kb_push(KEY_CTRL_UP);
            else
                kb_push(KEY_ARR_UP);
            return;
        case 0x50:
            if (shift)
                kb_push(KEY_SHIFT_DOWN);
            else if (ctrl)
                kb_push(KEY_CTRL_DOWN);
            else
                kb_push(KEY_ARR_DOWN);
            return;
        case 0x4B:
            if (shift)
                kb_push(KEY_SHIFT_LEFT);
            else if (ctrl)
                kb_push(KEY_CTRL_LEFT);
            else
                kb_push(KEY_ARR_LEFT);
            return;
        case 0x4D:
            if (shift)
                kb_push(KEY_SHIFT_RIGHT);
            else if (ctrl)
                kb_push(KEY_CTRL_RIGHT);
            else
                kb_push(KEY_ARR_RIGHT);
            return;
        case 0x53:
            if (ctrl)
                kb_push(KEY_CTRL_DELETE);
            else if (shift)
                kb_push(KEY_SHIFT_DELETE);
            else
                kb_push(KEY_DELETE);
            return;
        case 0x35:
            if (ctrl || shift)
                return;
            kb_push('/');
            return;
        default:
            return;
        }
    }
    else
    {
        switch (scancode)
        {
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4B:
        case 0x4C:
        case 0x4D:
        case 0x4F:
        case 0x50:
        case 0x51:
        case 0x52:
            return;
        }

        switch (scancode)
        {
        case 0x1C:
            if (ctrl || shift)
                return;
            kb_push('\n');
            return;
        case 0x0E:
            if (ctrl)
                kb_push(KEY_CTRL_BACKSPACE);
            else if (shift)
                kb_push(KEY_SHIFT_BACKSPACE);
            else
                kb_push('\b');
            return;
        case 0x0F:
            if (ctrl || shift)
                return;
            kb_push('\t');
            return;
        case 0x01:
            kb_push(KEY_ESCAPE);
            return;
        case 0x37:
            if (ctrl || shift)
                return;
            kb_push('*');
            return;
        case 0x4A:
            if (ctrl || shift)
                return;
            kb_push('-');
            return;
        case 0x4E:
            if (ctrl || shift)
                return;
            kb_push('+');
            return;
        case 0x53:
            if (ctrl || shift)
                return;
            kb_push('.');
            return;
        }

        if (ctrl)
        {
            switch (scancode)
            {
            case 0x2D:
                kb_push(KEY_CTRL_X);
                return;
            case 0x2E:
                kb_push(KEY_CTRL_C);
                return;
            case 0x2F:
                kb_push(KEY_CTRL_V);
                return;
            default:
                return;
            }
        }

        char ch = shift ? shift_map[scancode] : normal[scancode];
        if (ch)
            kb_push((unsigned char)ch);
    }
}

unsigned char read(void)
{
    unsigned char out;
    while (1)
    {
        asm volatile("cli");
        if (kb_pop(&out))
        {
            asm volatile("sti");
            return (unsigned char)out;
        }
        asm volatile("sti; hlt");
    }
}

void keyboard_init(void)
{
    unsigned char mask = inb(PIC1_DATA);
    mask &= ~(1 << 1);
    outb(PIC1_DATA, mask);
}

void keyboard_interrupt_handler_wrapper(void)
{
    keyboard_interrupt_handler_c();
}