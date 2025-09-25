#include <stdio.h>

int main(void)
{
    printf("\033[2J");
    printf(
        "\n\033[1;33m--- AVAILABLE COMMANDS ---\033[0m\n\n"
        "  \033[1;33mwhere\033[0m     - Show current directory   "
        "\033[1;33mdirlist\033[0m     - List files and directories\n"
        "  \033[1;33mecho\033[0m      - Echo text to screen      "
        "\033[1;33mbanner\033[0m      - Display text in banner\n"
        "  \033[1;33mdate\033[0m      - Show date and time       "
        "\033[1;33mdirmake\033[0m     - Create directory\n"
        "  \033[1;33mdirchange\033[0m - Change directory         "
        "\033[1;33mdirdelete\033[0m   - Delete directory\n"
        "  \033[1;33mfilemake\033[0m  - Create file              "
        "\033[1;33mfiledelete\033[0m  - Delete file\n"
        "  \033[1;33mfilewrite\033[0m - Write text into file     "
        "\033[1;33mfileread\033[0m    - Read text from file\n"
        "  \033[1;33mfilecopy\033[0m  - Make copy of file        "
        "\033[1;33mfilemove\033[0m    - Move or rename file\n"
        "  \033[1;33mfileinfo\033[0m  - Show file information    "
        "\033[1;33mfilecount\033[0m   - Count lines,words,chars\n"
        "  \033[1;33mfiledump\033[0m  - Show hex dump of file    "
        "\033[1;33mfilesearch\033[0m  - Search text in file\n"
        "  \033[1;33mmeminfo\033[0m   - Show heap memory info    "
        "\033[1;33msysinfo\033[0m     - Show system info\n"
        "  \033[1;33mclear\033[0m     - Clear the screen         "
        "\033[1;33mshutdown\033[0m    - Shutdown the system\n"
        "  \033[1;33mrestart\033[0m   - Restart the system       "
        "\033[1;33mwhatis\033[0m      - Show detailed command info\n"
        "  \033[1;33mfilediff\033[0m  - Compare file contents    "
        "\033[1;33muptime\033[0m      - Show time since boot\n\n");

    printf("To see what any command does, type '\033[1;33mwhatis\033[0m' followed by the command name without any arguments.\n");

    return 0;
}
