#ifndef BAOS_STDIO_FILE_INTERNAL_H
#define BAOS_STDIO_FILE_INTERNAL_H

#include <stdio.h>

#define MAX_OPEN_FILES 16
#define FILE_IO_CHUNK 512u

FILE *alloc_file_slot(void);
void free_file_slot(FILE *f);
int file_slot_index(FILE *f);
int file_refill_read(FILE *f);
unsigned int file_read_size(FILE *f);
void file_set_read_size(FILE *f, unsigned int size);
unsigned int file_read_chunk_off(FILE *f);
void file_set_read_chunk_off(FILE *f, unsigned int off);
extern int stdin_ungetc;

#endif
