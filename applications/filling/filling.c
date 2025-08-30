#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_LINES 1000
#define MAX_COL 1024

static char buf[MAX_LINES][MAX_COL];
static int lines = 0;

/*static void print_buffer(void)
{
    int i;
    for (i = 0; i < lines; i++)
        printf("%4d  %s", i + 1, buf[i]);
}

static int load_file(const char *fn)
{
    FILE *f = fopen(fn, "r");
    if (!f)
        return 0;
    lines = 0;
    while (lines < MAX_LINES && fgets(buf[lines], MAX_COL, f))
    {
        if (buf[lines][0] == 0)
            break;
        lines++;
    }
    fclose(f);
    return 1;
}

static int save_file(const char *fn)
{
    FILE *f = fopen(fn, "w");
    if (!f)
        return 0;
    int i;
    for (i = 0; i < lines; i++)
        fprintf(f, "%s", buf[i]);
    fclose(f);
    return 1;
}

static void insert_line(int pos, const char *text)
{
    if (pos < 0)
        pos = 0;
    if (pos > lines)
        pos = lines;
    if (lines >= MAX_LINES)
        return;
    int i;
    for (i = lines; i > pos; i--)
        strcpy(buf[i], buf[i - 1]);
    strncpy(buf[pos], text, MAX_COL - 1);
    buf[pos][MAX_COL - 1] = '\0';
    if (strlen(buf[pos]) == 0 || buf[pos][strlen(buf[pos]) - 1] != '\n')
    {
        size_t l = strlen(buf[pos]);
        if (l < MAX_COL - 1)
        {
            buf[pos][l] = '\n';
            buf[pos][l + 1] = '\0';
        }
    }
    lines++;
}

static void delete_line(int pos)
{
    if (pos < 0 || pos >= lines)
        return;
    int i;
    for (i = pos; i < lines - 1; i++)
        strcpy(buf[i], buf[i + 1]);
    buf[lines - 1][0] = '\0';
    lines--;
}*/

int main(int argc, char **argv)
{
    /*char cmd[128];
    char arg[1024];
    if (argc > 1)
        load_file(argv[1]);
    printf("Simple C editor. Commands: o <file>, s <file>, p, i <n>, a, d <n>, q\n");
    while (1)
    {
        printf("> ");
        if (!fgets(cmd, sizeof cmd, stdin))
            break;
        int i = 0;
        while (cmd[i] && isspace((unsigned char)cmd[i]))
            i++;
        if (cmd[i] == 0)
            continue;
        char c = cmd[i];
        if (c == 'q')
            break;
        if (c == 'p')
        {
            print_buffer();
            continue;
        }
        if (c == 'o')
        {
            while (cmd[i] && !isspace((unsigned char)cmd[i]))
                i++;
            while (cmd[i] && isspace((unsigned char)cmd[i]))
                i++;
            strncpy(arg, cmd + i, sizeof arg - 1);
            arg[sizeof arg - 1] = 0;
            size_t ln = strlen(arg);
            if (ln && arg[ln - 1] == '\n')
                arg[ln - 1] = 0;
            if (load_file(arg))
                printf("Loaded %s (%d lines)\n", arg, lines);
            else
                printf("Failed to open %s\n", arg);
            continue;
        }
        if (c == 's')
        {
            while (cmd[i] && !isspace((unsigned char)cmd[i]))
                i++;
            while (cmd[i] && isspace((unsigned char)cmd[i]))
                i++;
            strncpy(arg, cmd + i, sizeof arg - 1);
            arg[sizeof arg - 1] = 0;
            size_t ln = strlen(arg);
            if (ln && arg[ln - 1] == '\n')
                arg[ln - 1] = 0;
            if (save_file(arg))
                printf("Saved %s\n", arg);
            else
                printf("Failed to save %s\n", arg);
            continue;
        }
        if (c == 'i')
        {
            int num = 0;
            if (sscanf(cmd + i, "i %d", &num) >= 1)
                num--;
            printf("Enter line text (single line). End with newline.\n");
            if (!fgets(arg, sizeof arg, stdin))
                continue;
            insert_line(num, arg);
            continue;
        }
        if (c == 'a')
        {
            printf("Enter line text to append.\n");
            if (!fgets(arg, sizeof arg, stdin))
                continue;
            insert_line(lines, arg);
            continue;
        }
        if (c == 'd')
        {
            int num = 0;
            if (sscanf(cmd + i, "d %d", &num) >= 1)
            {
                delete_line(num - 1);
            }
            continue;
        }
        printf("Unknown command\n");
    }*/
    return 0;
}
