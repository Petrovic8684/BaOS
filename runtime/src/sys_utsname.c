#include <sys_utsname.h>
#include <string.h>

#define SYS_OS_NAME 6
#define SYS_KERNEL_VERSION 7

int uname(struct utsname *buf)
{
  if (!buf)
    return -1;

  char os_name[64] = {0};
  char kernel_version[32] = {0};

  asm volatile(
      "movl %[num], %%eax\n\t"
      "movl %[buf], %%ebx\n\t"
      "movl %[size], %%ecx\n\t"
      "int $0x80\n\t"
      :
      : [num] "i"(SYS_OS_NAME),
        [buf] "b"(os_name),
        [size] "c"(sizeof(os_name))
      : "eax", "memory");

  asm volatile(
      "movl %[num], %%eax\n\t"
      "movl %[buf], %%ebx\n\t"
      "movl %[size], %%ecx\n\t"
      "int $0x80\n\t"
      :
      : [num] "i"(SYS_KERNEL_VERSION),
        [buf] "b"(kernel_version),
        [size] "c"(sizeof(kernel_version))
      : "eax", "memory");

  strncpy(buf->sysname, os_name, sizeof(buf->sysname) - 1);
  buf->sysname[sizeof(buf->sysname) - 1] = '\0';

  strncpy(buf->nodename, "localhost", sizeof(buf->nodename) - 1);
  buf->nodename[sizeof(buf->nodename) - 1] = '\0';

  strncpy(buf->release, kernel_version, sizeof(buf->release) - 1);
  buf->release[sizeof(buf->release) - 1] = '\0';

  strncpy(buf->version, kernel_version, sizeof(buf->version) - 1);
  buf->version[sizeof(buf->version) - 1] = '\0';

  strncpy(buf->machine, "i386", sizeof(buf->machine) - 1);
  buf->machine[sizeof(buf->machine) - 1] = '\0';

  return 0;
}
