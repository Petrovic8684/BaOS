#include "dirent/dirent_internal.h"
#include "internal/fs_helpers.h"
#include <stdlib.h>
#include <string.h>

struct DIR dir_streams[DIRENT_MAX_STREAMS];

int find_free_stream(void)
{
    for (int i = 0; i < DIRENT_MAX_STREAMS; i++)
        if (!dir_streams[i].in_use)
            return i;

    return -1;
}

unsigned int parse_list_into_dir(struct DIR *d, const char *list)
{
    d->count = 0;
    d->pos = 0;

    if (!list)
        return 0;

    if (list[0] == '(')
        return 0;

    unsigned int out = 0;
    unsigned int i = 0;
    while (list[i] != '\0' && out < DIRENT_MAX_ENTRIES)
    {
        while (list[i] == ' ' || list[i] == '\t' || list[i] == '\r' || list[i] == '\n')
            i++;
        if (list[i] == '\0')
            break;

        char temp[DIRENT_NAME_MAX + 2];
        unsigned int j = 0;
        while (list[i] != '\0' && list[i] != ' ' && list[i] != '\t' && list[i] != '\r' && list[i] != '\n' && j < (DIRENT_NAME_MAX + 1))
            temp[j++] = list[i++];
        temp[j] = '\0';

        while (list[i] != '\0' && list[i] != ' ' && list[i] != '\t' && list[i] != '\r' && list[i] != '\n')
            i++;

        if (temp[0] == '\0')
            continue;

        unsigned char typ = DT_UNKNOWN;
        unsigned int actual_len = j;
        if (actual_len > 0 && temp[actual_len - 1] == '/')
        {
            typ = DT_DIR;
            temp[actual_len - 1] = '\0';
        }
        else
            typ = DT_REG;

        unsigned int k = 0;
        for (; k < DIRENT_NAME_MAX && temp[k] != '\0'; k++)
            d->names[out][k] = temp[k];
        d->names[out][k] = '\0';

        d->types[out] = typ;

        if (d->names[out][0] != '\0')
            out++;
    }

    d->count = out;
    return out;
}
