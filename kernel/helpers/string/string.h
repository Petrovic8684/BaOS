#ifndef STRING_H
#define STRING_H

int str_equal(const char *a, const char *b);
int str_count(const char *a);
void itoa(unsigned char num, char *str);
const char *uint_to_str(unsigned int val);
void str_copy_fixed(char *dst, const char *src, unsigned int max_len);

#endif