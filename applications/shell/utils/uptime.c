#include <sys/sysinfo.h>
#include <stdio.h>

struct time_unit
{
    const char *name;
    unsigned long seconds;
};

int main(void)
{
    struct sysinfo info;
    if (sysinfo(&info) != 0)
    {
        printf("\033[31mError: Failed to get system uptime.\033[0m\n");
        return 1;
    }

    unsigned long ms = info.uptime;
    unsigned long seconds = ms / 1000UL;

    struct time_unit units[] = {
        {"year", 365UL * 24 * 3600},
        {"month", 30UL * 24 * 3600},
        {"day", 24UL * 3600},
        {"hour", 3600},
        {"minute", 60},
        {"second", 1},
    };

    printf("\033[1;33mTime since boot:\033[0m ");

    int printed = 0;
    for (int i = 0; i < (int)(sizeof(units) / sizeof(units[0])); i++)
    {
        unsigned long count = seconds / units[i].seconds;
        seconds %= units[i].seconds;

        if (count == 0)
            continue;

        if (printed)
            printf(", ");
        printf("%lu %s%s", count, units[i].name, count > 1 ? "s" : "");
        printed = 1;
    }

    if (!printed)
    {
        if (ms == 0)
            printf("0 seconds");
        else
            printf("%lu ms", ms);
    }

    printf("\n");
    return 0;
}
