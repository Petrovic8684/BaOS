#include <stdio.h>

int main(int argc, char *argv[])
{
    if (argc <= 1)
    {
        printf("\033[31mError: Invalid text provided.\033[0m\n");
        return 1;
    }

    for (int i = 1; i < argc; ++i)
    {
        if (i > 1)
            putchar(' ');
        fputs(argv[i], stdout);
    }
    putchar('\n');

    return 0;
}
