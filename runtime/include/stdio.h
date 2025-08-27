#ifndef STDIO_H
#define STDIO_H

#include "../user_syscalls.h"
#include <stdarg.h>
#include <stddef.h>

#define EOF -1
#define BUFSIZE 256

typedef struct _FILE
{
    const char *name;
    unsigned int mode; // 0 = read, 1 = write
    unsigned int pos;
    unsigned char buf[BUFSIZE];
    unsigned int buf_pos;
    unsigned int buf_end;
} FILE;

#define stdin ((FILE *)0)
#define stdout ((FILE *)1)
#define stderr ((FILE *)2)

#define getc(stream) fgetc(stream)

int fputc(int c, FILE *stream);
int putchar(int c);
int fputs(const char *s, FILE *stream);
int puts(const char *s);
int vfprintf(FILE *stream, const char *fmt, va_list args);
int fprintf(FILE *stream, const char *fmt, ...);
int printf(const char *fmt, ...);
int getchar(void);
int ungetc(int c, FILE *stream);
int fgetc(FILE *stream);
void read_line(char *buf, int max_len);
int scanf(const char *fmt, ...);
int fscanf(FILE *stream, const char *fmt, ...);
int sscanf(const char *str, const char *fmt, ...);
int fflush(FILE *stream);

#endif
