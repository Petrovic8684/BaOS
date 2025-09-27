#ifndef MOUSE_H
#define MOUSE_H

typedef enum
{
    MOUSE_EV_MOVE,
    MOUSE_EV_BUTTON,
    MOUSE_EV_WHEEL
} mouse_event_type_t;

typedef struct
{
    mouse_event_type_t type;
    union
    {
        struct
        {
            int dx, dy;
            unsigned char buttons;
        } move;
        struct
        {
            unsigned char button;
            unsigned char pressed;
            int x, y;
        } button;
        struct
        {
            int delta;
            int x, y;
        } wheel;
    };
} mouse_event_t;

void mouse_irq_handler(int irq);

void mouse_init(void);
int mouse_read_event(mouse_event_t *out);
int mouse_peek_event(mouse_event_t *out);
void mouse_get_position(int *x, int *y);
int mouse_has_wheel(void);

#endif