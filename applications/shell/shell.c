#include "./utils/common/fs_common.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#define SYS_LOAD_USER_PROGRAM 17

static char *copy_into_argbuf(char *argbuf, size_t bufsize, size_t *used, const char *s)
{
    if (!s)
        return NULL;

    size_t slen = strlen(s);

    if (*used + slen + 1 > bufsize)
        return NULL;

    char *dst = argbuf + *used;

    memcpy(dst, s, slen);
    dst[slen] = '\0';

    *used += slen + 1;
    return dst;
}

static void run(const char *name, const char *args)
{
    static const char *argv[64];
    static char argbuf[2048];
    int argc = 0;
    size_t used = 0;

    argbuf[0] = '\0';

    char *pname = copy_into_argbuf(argbuf, sizeof(argbuf), &used, name ? name : "");
    if (!pname)
    {
        printf("\033[31mError: program name too long.\033[0m\n");
        return;
    }
    argv[argc++] = pname;

    if (args && args[0] != '\0')
    {
        char *pargs = copy_into_argbuf(argbuf, sizeof(argbuf), &used, args);
        if (!pargs)
        {
            printf("\033[31mError: args too long.\033[0m\n");
            return;
        }

        char *tok = strtok(pargs, " ");
        while (tok != NULL && argc < (int)(sizeof(argv) / sizeof(argv[0])) - 1)
        {
            argv[argc++] = tok;
            tok = strtok(NULL, " ");
        }
    }

    argv[argc] = NULL;

    asm volatile(
        "movl %[num], %%eax\n\t"
        "movl %[args], %%ebx\n\t"
        "movl %[prog], %%ecx\n\t"
        "int $0x80\n\t"
        :
        : [num] "i"(SYS_LOAD_USER_PROGRAM),
          [prog] "r"(argv[0]),
          [args] "r"(argv)
        : "eax", "ebx", "ecx", "memory");
}

void process_command(char *cmd)
{
    char command[32] = {0};

    while (*cmd == ' ')
        ++cmd;

    if (*cmd == '\0')
        return;

    char *p = cmd;
    size_t i = 0;
    while (*p != '\0' && *p != ' ' && i < sizeof(command) - 1)
        command[i++] = *p++;

    command[i] = '\0';

    while (*p == ' ')
        ++p;
    char *args = (*p != '\0') ? p : NULL;

    const char *paths[] = {"/programs", "/programs/utils"};
    bool launched = false;

    for (int pidx = 0; pidx < 2 && !launched; ++pidx)
    {
        DIR *d = opendir(paths[pidx]);
        if (!d)
            continue;

        struct dirent *ent;
        char prog_path[512];

        while ((ent = readdir(d)) != NULL)
        {
            if (strcmp(ent->d_name, command) != 0)
                continue;

            if (snprintf(prog_path, sizeof(prog_path), "%s/%s", paths[pidx], ent->d_name) >= (int)sizeof(prog_path))
                continue;

            closedir(d);
            launched = true;

            run(prog_path, args && args[0] != '\0' ? args : NULL);
            break;
        }

        if (!launched)
            closedir(d);
    }

    if (!launched)
        printf("\033[31mError: Unknown command. Type 'help' for a list of valid commands.\033[0m\n\n");
}

void main(void)
{
    char buffer[80];

    while (true)
    {
        printf("BaOS %s> ", get_current_dir_name());
        read_line(buffer, 80);
        printf("\n");
        process_command(buffer);
    }
}