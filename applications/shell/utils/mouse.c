#include <stdio.h>
#include <unistd.h>
#include <baos/mouse.h>

int main(void)
{
    printf("\033[2J");

    printf("\n\033[1;33m--- MOUSE TEST ---\033[0m\n");
    printf("Exit program by pressing wheel mouse button.\n\n");

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
                printf("Moved to (%d,%d)\n", x, y);
                break;
            }
            case MOUSE_EV_BUTTON:
                if (ev.button.button == 4 && ev.button.pressed == 1)
                {
                    printf("\033[2J");
                    return 0;
                }

                char *button;
                if (ev.button.button == 1)
                    button = "Left";
                else if (ev.button.button == 2)
                    button = "Right";
                else
                    button = "Unknown";

                printf("%s button %s at (%d,%d)\n", button, ev.button.pressed ? "pressed" : "released", ev.button.x, ev.button.y);
                break;
            case MOUSE_EV_WHEEL:
                printf("Scrolled %s at (%d,%d)\n", ev.wheel.delta == 1 ? "down" : "up", ev.wheel.x, ev.wheel.y);
                break;
            }
        }

        usleep(5000);
    }

    return 0;
}
