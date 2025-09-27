#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

typedef struct
{
    int quot;
    int rem;
} div_t;

typedef struct
{
    long quot;
    long rem;
} ldiv_t;

typedef struct
{
    long long quot;
    long long rem;
} lldiv_t;

void exit(int code);
void abort(void);
double strtod(const char *nptr, char **endptr);
unsigned long strtoul(const char *nptr, char **endptr, int base);
long strtol(const char *nptr, char **endptr, int base);
void *malloc(unsigned int size);
void free(void *ptr);
void *realloc(void *ptr, unsigned int size);
void *calloc(unsigned int nmemb, unsigned int size);
void srand(unsigned int seed);
int rand(void);
double atof(const char *nptr);
int atoi(const char *nptr);
long atol(const char *nptr);
int abs(int n);
long labs(long n);
long long llabs(long long n);
div_t div(int numer, int denom);
ldiv_t ldiv(long numer, long denom);
lldiv_t lldiv(long long numer, long long denom);
void *bsearch(const void *key, const void *base, size_t nitems, size_t size, int (*compar)(const void *, const void *));
void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));
char *realpath(const char *path, char *resolved_path);

#endif
