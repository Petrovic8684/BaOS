#include <stdio.h>
#include <unistd.h>
#include <baos/mouse.h>

int main(void)
{
    printf("== Mouse test ==\n");
    printf("Exit program by pressing wheel mouse button.\n");

    mouse_event_t ev;
    while (mouse_peek(&ev))
        mouse_read(&ev);

    while (1)
    {
        if (mouse_read(&ev))
        {
            switch (ev.type)
            {
            case MOUSE_EV_MOVE:
            {
                int x, y;
                mouse_getpos(&x, &y);
                printf("Move to: x=%d y=%d buttons=0x%02X\n", x, y, ev.move.buttons);
                break;
            }
            case MOUSE_EV_BUTTON:
                printf("Button %u %s at (%d,%d)\n", ev.button.button, ev.button.pressed ? "pressed" : "released", ev.button.x, ev.button.y);
                if (ev.button.button == 4 && ev.button.pressed == 1)
                {
                    printf("Right button pressed. Exiting.\n");
                    return 0;
                }
                break;
            case MOUSE_EV_WHEEL:
                printf("Wheel: delta=%d at (%d,%d)\n", ev.wheel.delta, ev.wheel.x, ev.wheel.y);
                break;
            }
        }

        usleep(5000);
    }

    return 0;
}
