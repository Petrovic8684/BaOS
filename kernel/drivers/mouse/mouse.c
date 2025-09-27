#include "mouse.h"
#include "../display/display.h"
#include "../../helpers/ports/ports.h"
#include "../pit/pit.h"

#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_CMD_PORT 0x64

#define PS2_CMD_ENABLE_AUX 0xA8
#define PS2_CMD_READ_CMD_BYTE 0x20
#define PS2_CMD_WRITE_CMD_BYTE 0x60
#define PS2_CMD_WRITE_TO_AUX 0xD4

#define MOUSE_CMD_ENABLE_DATA 0xF4
#define MOUSE_CMD_SET_SAMPLE 0xF3
#define MOUSE_CMD_GET_ID 0xF2
#define MOUSE_CMD_RESET 0xFF

#define PS2_ACK 0xFA
#define PS2_RESEND 0xFE

#define MOUSE_BUF_SIZE 128
static volatile mouse_event_t mouse_buf[MOUSE_BUF_SIZE];
static volatile unsigned int mouse_head = 0;
static volatile unsigned int mouse_tail = 0;
static volatile unsigned int mouse_count = 0;

#define MAX_PACKET 4
static volatile unsigned char pkt[MAX_PACKET];
static volatile unsigned int pkt_index = 0;
static unsigned int packet_size = 3;

static volatile int mouse_x = 0;
static volatile int mouse_y = 0;

#define PS2_WAIT_MS 1000U
#define PS2_RETRY_MAX 8

static void ps2_flush_output(void)
{
    int safety = 0;
    while ((inb(PS2_STATUS_PORT) & 0x01) && (safety++ < 64))
        (void)inb(PS2_DATA_PORT);
}

static int ps2_wait_input_timeout(unsigned int ms)
{
    unsigned int waited = 0;
    while (inb(PS2_STATUS_PORT) & 0x02)
    {
        if (waited >= ms)
            return -1;

        pit_sleep(1);
        waited++;
    }
    return 0;
}

static int ps2_wait_output_timeout(unsigned int ms)
{
    unsigned int waited = 0;
    while (!(inb(PS2_STATUS_PORT) & 0x01))
    {
        if (waited >= ms)
            return -1;

        pit_sleep(1);
        waited++;
    }
    return 0;
}

static int ps2_mouse_send_cmd(unsigned char cmd, unsigned char *ack_out, unsigned char *data_out, int data_len)
{
    unsigned char resp = 0;
    int attempt;

    ps2_flush_output();

    for (attempt = 0; attempt < PS2_RETRY_MAX; ++attempt)
    {
        if (ps2_wait_input_timeout(PS2_WAIT_MS) != 0)
        {
            write("Input buffer busy before D4.\n");
            return -1;
        }

        outb(PS2_CMD_PORT, PS2_CMD_WRITE_TO_AUX);
        if (ps2_wait_input_timeout(PS2_WAIT_MS) != 0)
        {
            write("Input buffer busy after D4.\n");
            return -1;
        }

        outb(PS2_DATA_PORT, cmd);

        if (ps2_wait_output_timeout(PS2_WAIT_MS) != 0)
        {
            write("Timeout waiting for ACK.\n");
            return -1;
        }

        resp = inb(PS2_DATA_PORT);
        if (ack_out)
            *ack_out = resp;

        if (resp == PS2_ACK)
            break;

        else if (resp == PS2_RESEND)
        {
            write("Got RESEND (0xFE), retrying...\n");
            pit_sleep(10);
            continue;
        }
        else
        {
            write("Unexpected response: ");
            write_hex(resp);
            write("\n");
            return -1;
        }
    }

    if (attempt >= PS2_RETRY_MAX && resp != PS2_ACK)
    {
        write("Max retries exceeded.\n");
        return -1;
    }

    for (int i = 0; i < data_len; ++i)
    {
        if (ps2_wait_output_timeout(PS2_WAIT_MS) != 0)
        {
            write("Timeout reading data byte.\n");
            return -1;
        }
        unsigned char v = inb(PS2_DATA_PORT);
        if (data_out)
            data_out[i] = v;
        else
            (void)v;
    }

    ps2_flush_output();
    return 0;
}

static void push_public_event(const mouse_event_t *ev)
{
    unsigned int next = (mouse_head + 1) & (MOUSE_BUF_SIZE - 1);
    if (next == mouse_tail)
    {
        mouse_tail = (mouse_tail + 1) & (MOUSE_BUF_SIZE - 1);
        mouse_count--;
    }
    mouse_buf[mouse_head] = *ev;
    mouse_head = next;
    mouse_count++;
}

static int pop_public_event(mouse_event_t *out)
{
    if (mouse_count == 0)
        return 0;
    *out = mouse_buf[mouse_tail];
    mouse_tail = (mouse_tail + 1) & (MOUSE_BUF_SIZE - 1);
    mouse_count--;
    return 1;
}

void mouse_irq_handler(int irq)
{
    (void)irq;
    unsigned char data = inb(PS2_DATA_PORT);

    if (pkt_index == 0)
        if (!(data & 0x08))
        {
            outb(PIC2_CMD, 0x20);
            outb(PIC1_CMD, 0x20);
            return;
        }

    pkt[pkt_index++] = data;

    if (pkt_index >= (unsigned)packet_size)
    {
        unsigned char b0 = pkt[0];
        unsigned char b1 = pkt[1];
        unsigned char b2 = pkt[2];

        int dx = (signed char)b1;
        int dy = (signed char)b2;

        if (b0 & 0x40)
        {
            if (dx > 0)
                dx = 127;
            else
                dx = -128;
        }
        if (b0 & 0x80)
        {
            if (dy > 0)
                dy = 127;
            else
                dy = -128;
        }

        mouse_x += dx;
        mouse_y -= dy;

        mouse_event_t ev;
        ev.type = MOUSE_EV_MOVE;
        ev.move.dx = dx;
        ev.move.dy = dy;
        ev.move.buttons = b0 & 0x07;
        push_public_event(&ev);

        static unsigned char last_buttons = 0;
        unsigned char cur_buttons = b0 & 0x07;
        unsigned char changed = cur_buttons ^ last_buttons;
        if (changed)
        {
            for (int mask = 1; mask <= 4; mask <<= 1)
            {
                if (changed & mask)
                {
                    mouse_event_t bev;
                    bev.type = MOUSE_EV_BUTTON;
                    bev.button.button = (unsigned char)mask;
                    bev.button.pressed = (cur_buttons & mask) ? 1 : 0;
                    bev.button.x = mouse_x;
                    bev.button.y = mouse_y;
                    push_public_event(&bev);
                }
            }
            last_buttons = cur_buttons;
        }

        if (packet_size == 4)
        {
            int wheel = (signed char)pkt[3];
            if (wheel != 0)
            {
                mouse_event_t wev;
                wev.type = MOUSE_EV_WHEEL;
                wev.wheel.delta = wheel;
                wev.wheel.x = mouse_x;
                wev.wheel.y = mouse_y;
                push_public_event(&wev);
            }
        }

        pkt_index = 0;
    }

    outb(PIC2_CMD, 0x20);
    outb(PIC1_CMD, 0x20);
}

int mouse_read_event(mouse_event_t *out)
{
    mouse_event_t ev;
    while (1)
    {
        __asm__ volatile("cli");
        if (pop_public_event(&ev))
        {
            __asm__ volatile("sti");
            if (out)
                *out = ev;
            return 1;
        }
        __asm__ volatile("sti\n\thlt\n\t");
    }
    return 0;
}

int mouse_peek_event(mouse_event_t *out)
{
    int ret = 0;
    __asm__ volatile("cli");
    if (pop_public_event(out))
        ret = 1;
    __asm__ volatile("sti");
    return ret;
}

void mouse_get_position(int *x, int *y)
{
    __asm__ volatile("cli");
    if (x)
        *x = mouse_x;
    if (y)
        *y = mouse_y;
    __asm__ volatile("sti");
}

int mouse_has_wheel(void)
{
    return (packet_size == 4) ? 1 : 0;
}

void mouse_init(void)
{
    write("Initializing PS/2 mouse driver...\n");
    ps2_flush_output();

    __asm__ volatile("cli");
    unsigned char mask2 = inb(PIC2_DATA);
    mask2 |= (1 << (12 - 8));
    outb(PIC2_DATA, mask2);
    __asm__ volatile("sti");

    if (ps2_wait_input_timeout(PS2_WAIT_MS) != 0)
        write("Input buffer busy before ENABLE_AUX.\n");
    outb(PS2_CMD_PORT, PS2_CMD_ENABLE_AUX);

    unsigned char cmd = 0;

    if (ps2_wait_input_timeout(PS2_WAIT_MS) != 0)
        write("Timeout before READ_CMD_BYTE.\n");
    outb(PS2_CMD_PORT, PS2_CMD_READ_CMD_BYTE);

    cmd = inb(PS2_DATA_PORT);

    cmd |= 0x02;
    cmd &= ~(0x20);

    if (ps2_wait_input_timeout(PS2_WAIT_MS) != 0)
        write("Timeout before WRITE_CMD_BYTE.\n");
    outb(PS2_CMD_PORT, PS2_CMD_WRITE_CMD_BYTE);

    if (ps2_wait_input_timeout(PS2_WAIT_MS) != 0)
        write("Timeout before writing cmdbyte data.\n");
    outb(PS2_DATA_PORT, cmd);

    unsigned char ack;
    unsigned char idbuf[1];
    int ok;

    ok = ps2_mouse_send_cmd(MOUSE_CMD_SET_SAMPLE, &ack, ((void *)0), 0);
    if (ok != 0)
        write("No ACK for SET_SAMPLE (1).\n");

    ok = ps2_mouse_send_cmd(200, &ack, ((void *)0), 0);
    if (ok != 0)
        write("No ACK for sample=200.\n");

    ok = ps2_mouse_send_cmd(MOUSE_CMD_SET_SAMPLE, &ack, ((void *)0), 0);
    if (ok != 0)
        write("No ACK for SET_SAMPLE (2).\n");

    ok = ps2_mouse_send_cmd(100, &ack, ((void *)0), 0);
    if (ok != 0)
        write("No ACK for sample=100.\n");

    ok = ps2_mouse_send_cmd(MOUSE_CMD_SET_SAMPLE, &ack, ((void *)0), 0);
    if (ok != 0)
        write("No ACK for SET_SAMPLE (3).\n");

    ok = ps2_mouse_send_cmd(80, &ack, ((void *)0), 0);
    if (ok != 0)
        write("No ACK for sample=80.\n");

    ok = ps2_mouse_send_cmd(MOUSE_CMD_GET_ID, &ack, idbuf, 1);
    if (ok != 0)
    {
        write("GET_ID timed out or no response. Assuming no wheel...\n");
        packet_size = 3;
    }
    else
    {
        unsigned char dev_id = idbuf[0];
        if (dev_id == 0x03)
        {
            packet_size = 4;
            write("Mouse wheel detected. 4-byte packets enabled.\n");
        }
        else
        {
            packet_size = 3;
            write("\033[1;33mWarning: PS/2 mouse wheel not detected (device id:");
            write_hex(dev_id);
            write("). Falling back to 3-byte mode.\033[0m\n");
        }
    }

    ok = ps2_mouse_send_cmd(MOUSE_CMD_ENABLE_DATA, &ack, ((void *)0), 0);

    __asm__ volatile("cli");
    mask2 = inb(PIC2_DATA);
    mask2 &= ~(1 << (12 - 8));
    outb(PIC2_DATA, mask2);

    unsigned char mask1 = inb(PIC1_DATA);
    mask1 &= ~(1 << 2);
    outb(PIC1_DATA, mask1);
    __asm__ volatile("sti");

    if (ok != 0 || ack != PS2_ACK)
    {
        mouse_x = 0;
        mouse_y = 0;

        write("\033[1;33mWarning: Mouse did not ACK enable-data (or timed out). Mouse may be missing.\033[0m\n\n");
        return;
    }
    else
        write("\033[32mPS/2 mouse data reporting enabled.\033[0m\n");

    mouse_x = 0;
    mouse_y = 0;

    write("\033[32mPS/2 mouse driver initialized.\033[0m\n\n");
}
