#include <stdio.h>

int main(void)
{
    printf("\033[2J");
    printf(
        "\nCommands:\n"
        "  where        - Show current directory\n"
        "  list         - List files and directories\n"
        "  echo         - Print text to the console\n"
        "  date         - Show current date and time\n"
        "  makedir      - Create directory\n"
        "  changedir    - Change directory\n"
        "  deletedir    - Delete directory\n"
        "  makefile     - Create file\n"
        "  deletefile   - Delete file\n"
        "  writefile    - Write text into file\n"
        "  readfile     - Read text from file\n"
        "  copy         - Copy file or directory\n"
        "  move         - Move file or directory\n"
        "  osname       - Show OS name\n"
        "  version      - Show kernel version\n"
        "  help         - Show this help message\n");

    return 0;
}
