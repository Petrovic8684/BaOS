#include <stdio.h>
#include "internal/syscalls.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "internal/fs_helpers.h"
#include "stdio/file_internal.h"

FILE *fopen(const char *pathname, const char *mode)
{
    if (!pathname || !mode)
    {
        errno = EINVAL;
        return NULL;
    }

    FILE *f = alloc_file_slot();
    if (!f)
    {
        errno = EMFILE;
        return NULL;
    }

    f->buf = NULL;
    f->buf_pos = 0;
    f->buf_end = 0;
    f->name = strdup(pathname);
    if (!f->name)
    {
        free_file_slot(f);
        return NULL;
    }
    f->eof = 0;
    f->err = 0;

    if (mode[0] == 'r')
    {
        f->mode = 0;
        int size = fs_read_file_size(f->name);
        if (size >= 0)
        {
            if (size > 0)
            {
                f->buf = (unsigned char *)malloc(size);
                if (!f->buf)
                {
                    free_file_slot(f);
                    return NULL;
                }
                unsigned int got = 0;
                if (fs_read_file(f->name, f->buf, size, &got) != 0)
                {
                    free_file_slot(f);
                    return NULL;
                }
                f->buf_pos = 0;
                f->buf_end = got;
            }
            else
            {
                f->buf = NULL;
                f->buf_pos = 0;
                f->buf_end = 0;
            }
        }
        else
        {
            free_file_slot(f);
            return NULL;
        }
    }
    else if (mode[0] == 'w')
    {
        f->mode = 1;
        int del_rc = fs_delete_file(f->name);
        int rc = fs_make_file(f->name);
        if (rc != 0)
        {
            free_file_slot(f);
            return NULL;
        }
    }
    else if (mode[0] == 'a')
    {
        f->mode = 1;
        int size = fs_read_file_size(f->name);
        if (size > 0)
        {
            f->buf = (unsigned char *)malloc(size);
            if (!f->buf)
            {
                free_file_slot(f);
                return NULL;
            }
            unsigned int got = 0;
            if (fs_read_file(f->name, f->buf, size, &got) != 0)
            {
                free_file_slot(f);
                return NULL;
            }
            f->buf_pos = got;
            f->buf_end = got;
        }
        else
        {
            int rc = fs_make_file(f->name);
            if (rc != 0)
            {
                free_file_slot(f);
                return NULL;
            }
            f->buf_pos = 0;
            f->buf_end = 0;
        }
    }
    else
    {
        errno = EINVAL;
        free_file_slot(f);
        return NULL;
    }

    return f;
}
