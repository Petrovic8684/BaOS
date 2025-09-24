#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TIMEZONE_FILE_PATH "/config/timezone"

int main(int argc, char *argv[])
{
    if (argc > 1 && strncmp(argv[1], "-zone=", 6) == 0)
    {
        int offset = atoi(argv[1] + 6);
        if (offset < -12 || offset > 14)
        {
            printf("\033[31mError: Input timezone is invalid.\033[0m\n");
            return 1;
        }

        FILE *f = fopen(TIMEZONE_FILE_PATH, "w");
        if (!f)
        {
            printf("\033[31mError: Failed to set timezone.\033[0m\n");
            return 1;
        }

        int wrote = fprintf(f, "%d\n", offset);
        fclose(f);

        if (wrote <= 0)
        {
            printf("\033[31mError: Failed to set timezone.\033[0m\n");
            return 1;
        }

        printf("\033[32mTimezone set to %d\033[0m\n", offset);
    }

    time_t t = time(NULL);
    struct tm *lt = localtime(&t);

    printf("\033[1;33mCurrent date:\033[0m %s", asctime(lt));

    return 0;
}
