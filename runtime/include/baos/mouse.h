#ifndef BAOS_MOUSE_H
#define BAOS_MOUSE_H

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

int mouse_read(mouse_event_t *ev);
int mouse_peek(mouse_event_t *ev);
void mouse_getpos(int *x, int *y);
int mouse_has_wheel(void);

#endif
