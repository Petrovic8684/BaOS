#ifndef STDLIB_H
#define STDLIB_H

void exit(int code);
double strtod(const char *nptr, char **endptr);
unsigned long strtoul(const char *nptr, char **endptr, int base);
long strtol(const char *nptr, char **endptr, int base);
void *malloc(unsigned int size);
void free(void *ptr);
void *realloc(void *ptr, unsigned int size);
void *calloc(unsigned int nmemb, unsigned int size);

#endif
