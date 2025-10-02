#ifndef ERRNO_H
#define ERRNO_H

int *__errno_location(void);
#define errno (*__errno_location())

#define EPERM 1
#define ENOENT 2
#define EIO 5
#define E2BIG 7
#define ENOEXEC 8
#define EAGAIN 11
#define ENOMEM 12
#define EACCES 13
#define EFAULT 14
#define EBUSY 16
#define EEXIST 17
#define ENODEV 19;
#define ENOTDIR 20
#define EISDIR 21
#define EINVAL 22
#define ENFILE 23
#define EMFILE 24
#define ETXTBSY 26
#define EFBIG 27
#define ENOSPC 28
#define ESPIPE 29
#define EROFS 30
#define EDOM 33
#define ERANGE 34
#define EDEADLK 35
#define ENAMETOOLONG 36
#define ENOSYS 38
#define ENOTEMPTY 39
#define ELOOP 40
#define EBFONT 59
#define ENODATA 61
#define ETIME 62
#define EILSEQ 84
#define ERESTART 85
#define EDQUOT 122

char *strerror(int errnum);
void perror(const char *s);
int map_fs_error(int fs_error);

#endif
