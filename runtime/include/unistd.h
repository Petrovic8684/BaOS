#ifndef UNISTD_H
#define UNISTD_H

#include <stddef.h>

typedef unsigned int useconds_t;

int rmdir(const char *path);
int chdir(const char *path);
char *getcwd(char *buf, size_t size);
unsigned int sleep(unsigned int seconds);
int usleep(useconds_t usec);

#endif
