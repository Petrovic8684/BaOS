#include <stdio.h>

int main(void)
{
    printf("\033[2J");
    printf(
        "\n\033[1;33mAVAILABLE COMMANDS:\033[0m\n\n"
        "  \033[1;33mwhere\033[0m      - Show current directory  "
        "\033[1;33mlist\033[0m       - List files and directories\n"
        "  \033[1;33mecho\033[0m       - Echo text to screen     "
        "\033[1;33mdate\033[0m       - Show current date and time\n"
        "  \033[1;33mmakedir\033[0m    - Create directory        "
        "\033[1;33mchangedir\033[0m  - Change directory\n"
        "  \033[1;33mdeletedir\033[0m  - Delete directory        "
        "\033[1;33mmakefile\033[0m   - Create file\n"
        "  \033[1;33mdeletefile\033[0m - Delete file             "
        "\033[1;33mwritefile\033[0m  - Write text into file\n"
        "  \033[1;33mreadfile\033[0m   - Read text from file     "
        "\033[1;33mcopy\033[0m       - Copy file or directory\n"
        "  \033[1;33mmove\033[0m       - Move file or directory  "
        "\033[1;33mosname\033[0m     - Show OS name\n"
        "  \033[1;33mversion\033[0m    - Show kernel version     "
        "\033[1;33mclear\033[0m      - Clear the screen\n"
        "  \033[1;33mfileinfo\033[0m   - Show file information   "
        "\033[1;33mshutdown\033[0m   - Shutdown the system\n"
        "  \033[1;33mrestart\033[0m    - Restart the system\n\n");

    printf("- To see what any command does, type '\033[1;33mwhatis\033[0m' followed by the command name without arguments.\n");

    return 0;
}
