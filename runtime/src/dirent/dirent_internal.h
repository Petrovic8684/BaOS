#ifndef BAOS_DIRENT_INTERNAL_H
#define BAOS_DIRENT_INTERNAL_H

#include <dirent.h>

struct DIR
{
    int in_use;
    char names[DIRENT_MAX_ENTRIES][DIRENT_NAME_MAX + 1];
    unsigned char types[DIRENT_MAX_ENTRIES];
    unsigned int count;
    long pos;
    struct dirent dent;
};

extern struct DIR dir_streams[DIRENT_MAX_STREAMS];

int find_free_stream(void);
unsigned int parse_list_into_dir(struct DIR *d, const char *list);

#endif
