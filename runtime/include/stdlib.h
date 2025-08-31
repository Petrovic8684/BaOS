#ifndef STDLIB_H
#define STDLIB_H

void exit(int code);
double strtod(const char *nptr, char **endptr);
unsigned long strtoul(const char *nptr, char **endptr, int base);

#endif
