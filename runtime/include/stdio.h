#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>
#include <stddef.h>

#define EOF -1
#define BUFSIZ 256

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct _FILE
{
    const char *name;
    unsigned int mode; /* 0 = read, 1 = write */
    unsigned int pos;
    unsigned char buf[BUFSIZ];
    unsigned int buf_pos;
    unsigned int buf_end;
    int eof;
    int err;
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

FILE *fopen(const char *pathname, const char *mode);
int fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
void rewind(FILE *stream);
int remove(const char *pathname);
int rename(const char *oldpath, const char *newpath);

int feof(FILE *stream);
int ferror(FILE *stream);
void clearerr(FILE *stream);

int setvbuf(FILE *stream, char *buf, int mode, size_t size);
void setbuf(FILE *stream, char *buf);

#endif
