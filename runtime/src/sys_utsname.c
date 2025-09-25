#include <sys/utsname.h>

#define SYS_UNAME 6

int uname(struct utsname *buf)
{
  __asm__ volatile(
      "movl %[num], %%eax\n\t"
      "movl %[arg], %%ebx\n\t"
      "int $0x80\n\t"
      :
      : [num] "i"(SYS_UNAME), [arg] "r"(buf)
      : "eax", "ebx", "memory");

  return 0;
}