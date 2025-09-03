#include <stdio.h>

int main(int argc, char *argv[])
{
    const char *arg1 = (argc > 1) ? argv[1] : NULL;
    const char *arg2 = (argc > 2) ? argv[2] : NULL;

    if ((!arg1 || arg1[0] == 0) && (!arg2 || arg2[0] == 0))
    {
        printf("\033[31mError: Invalid text provided.\033[0m\n");
        return 1;
    }

    if (arg1 && arg1[0] != 0)
        printf("%s", arg1);
    if (arg2 && arg2[0] != 0)
        printf(" %s", arg2);
    printf("\n");

    return 0;
}
