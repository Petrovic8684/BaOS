#ifndef UNISTD_H
#define UNISTD_H

#include <stddef.h>

int rmdir(const char *path);
int chdir(const char *path);
char *getcwd(char *buf, size_t size);

#endif
