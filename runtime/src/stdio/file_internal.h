#ifndef BAOS_STDIO_FILE_INTERNAL_H
#define BAOS_STDIO_FILE_INTERNAL_H

#include <stdio.h>

#define MAX_OPEN_FILES 16

FILE *alloc_file_slot(void);
void free_file_slot(FILE *f);
extern int stdin_ungetc;

#endif
