#include "stdio/file_internal.h"
#include "internal/fs_helpers.h"
#include <stdlib.h>
#include <string.h>

static FILE file_table[MAX_OPEN_FILES];
static unsigned int file_sizes[MAX_OPEN_FILES];
static unsigned int file_chunk_offs[MAX_OPEN_FILES];

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
            file_sizes[i] = 0;
            file_chunk_offs[i] = 0;
            return &file_table[i];
        }
    }
    return NULL;
}


int file_slot_index(FILE *f)
{
    if (!f)
        return -1;

    for (int i = 0; i < MAX_OPEN_FILES; i++)
    {
        if (&file_table[i] == f)
            return i;
    }

    return -1;
}


unsigned int file_read_size(FILE *f)
{
    int idx = file_slot_index(f);
    if (idx < 0)
        return 0;
    return file_sizes[idx];
}


void file_set_read_size(FILE *f, unsigned int size)
{
    int idx = file_slot_index(f);
    if (idx < 0)
        return;
    file_sizes[idx] = size;
}


unsigned int file_read_chunk_off(FILE *f)
{
    int idx = file_slot_index(f);
    if (idx < 0)
        return 0;
    return file_chunk_offs[idx];
}


void file_set_read_chunk_off(FILE *f, unsigned int off)
{
    int idx = file_slot_index(f);
    if (idx < 0)
        return;
    file_chunk_offs[idx] = off;
}


int file_refill_read(FILE *f)
{
    if (!f || !f->name || !f->buf)
        return -1;

    unsigned int file_size = file_read_size(f);
    if (f->pos >= file_size)
    {
        f->buf_end = 0;
        return 0;
    }

    unsigned int chunk = file_size - f->pos;
    if (chunk > FILE_IO_CHUNK)
        chunk = FILE_IO_CHUNK;

    unsigned int got = 0;
    if (fs_read_file_at(f->name, f->pos, f->buf, chunk, &got) != 0)
        return -1;

    file_set_read_chunk_off(f, f->pos);
    f->buf_end = got;
    return 0;
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

    int idx = file_slot_index(f);
    if (idx >= 0)
    {
        file_sizes[idx] = 0;
        file_chunk_offs[idx] = 0;
    }
}
