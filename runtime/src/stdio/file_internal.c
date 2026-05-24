#include "stdio/file_internal.h"
#include <stdlib.h>
#include <string.h>

#define MAX_OPEN_FILES 16

static FILE file_table[MAX_OPEN_FILES];

int stdin_ungetc = -1;


FILE *alloc_file_slot(void)
{
    for (int i = 0; i < MAX_OPEN_FILES; i++)
    {
        if (file_table[i].name == NULL)
        {
            file_table[i].mode = 0;
            file_table[i].pos = 0;
            file_table[i].buf = NULL;
            file_table[i].buf_pos = 0;
            file_table[i].buf_end = 0;
            file_table[i].eof = 0;
            file_table[i].err = 0;
            return &file_table[i];
        }
    }
    return NULL;
}


void free_file_slot(FILE *f)
{
    if (!f)
        return;

    if (f->buf)
    {
        free(f->buf);
        f->buf = NULL;
    }

    if (f->name)
    {
        free((void *)f->name);
        f->name = NULL;
    }

    f->mode = 0;
    f->pos = 0;
    f->buf_pos = 0;
    f->buf_end = 0;
    f->eof = 0;
    f->err = 0;
}
