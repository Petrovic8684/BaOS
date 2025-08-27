#include "history.h"

static char history[HISTORY_SIZE][80];
static int history_count = 0;
static int history_index = 0;

void history_init(void)
{
    history_count = 0;
    history_index = 0;
}

void history_add(const char *cmd)
{
    if (!cmd[0])
        return;

    if (history_count < HISTORY_SIZE)
    {
        strcpy(history[history_count], cmd);
        history_count++;
    }
    else
    {
        for (int i = 1; i < HISTORY_SIZE; i++)
            strcpy(history[i - 1], history[i]);

        strcpy(history[HISTORY_SIZE - 1], cmd);
    }

    history_index = history_count;
}

const char *history_prev(void)
{
    if (history_count == 0 || history_index == 0)
        return 0;

    history_index--;
    return history[history_index];
}

const char *history_next(void)
{
    if (history_count == 0 || history_index >= history_count)
        return 0;

    history_index++;
    if (history_index == history_count)
        return 0;

    return history[history_index];
}

void history_reset_index(void)
{
    history_index = history_count;
}
