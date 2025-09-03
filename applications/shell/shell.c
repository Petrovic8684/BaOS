#include "../utils/common/fs_common.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#define SYS_LOAD_USER_PROGRAM 17

static const char *get_current_dir_name(void)
{
    const char *path = where();
    if (!path || path[0] == '\0')
        return "/";

    return path_basename(path);
}

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

void run(const char *name, const char *args)
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
    char arg1[256] = {0};
    char arg2[256] = {0};

    char *token = strtok(cmd, " ");
    if (token)
        strncpy(command, token, sizeof(command) - 1);

    token = strtok(NULL, " ");
    if (token)
        strncpy(arg1, token, sizeof(arg1) - 1);

    token = strtok(NULL, "");
    if (token)
        strncpy(arg2, token, sizeof(arg2) - 1);

    if (command[0] == '\0')
        return;

    const char *paths[] = {"/programs", "/programs/utils"};
    bool launched = false;

    for (int p = 0; p < 2 && !launched; ++p)
    {
        DIR *d = opendir(paths[p]);
        if (!d)
            continue;

        struct dirent *ent;
        char prog_path[512];

        while ((ent = readdir(d)) != NULL)
        {
            if (strcmp(ent->d_name, command) != 0)
                continue;

            if (snprintf(prog_path, sizeof(prog_path), "%s/%s", paths[p], ent->d_name) >= (int)sizeof(prog_path))
                continue;

            closedir(d);
            launched = true;
            char args_combined[512] = {0};

            if (arg1[0] != '\0')
            {
                strcpy(args_combined, arg1);
                if (arg2[0] != '\0')
                {
                    strcat(args_combined, " ");
                    strcat(args_combined, arg2);
                }
            }
            else if (arg2[0] != '\0')
                strcpy(args_combined, arg2);

            run(prog_path, args_combined[0] != '\0' ? args_combined : NULL);
        }

        if (!launched)
            closedir(d);
    }

    if (!launched)
        printf("\033[31mError: Unknown command. Type 'help' for a list of valid commands.\033[0m\n");
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