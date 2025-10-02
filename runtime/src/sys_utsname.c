#include <sys/utsname.h>

#define SYS_SYS_INFO 6

int uname(struct utsname *buf)
{
  __asm__ volatile(
      "movl %[num], %%eax\n\t"
      "movl %[arg], %%ebx\n\t"
      "int $0x80\n\t"
      :
      : [num] "i"(SYS_SYS_INFO), [arg] "r"(buf)
      : "eax", "ebx", "memory");

  return 0;
}