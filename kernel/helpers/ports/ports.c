#include "ports.h"

void outb(unsigned short port, unsigned char val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

unsigned char inb(unsigned short port)
{
    unsigned char ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void outw(unsigned short port, unsigned short val)
{
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

unsigned short inw(unsigned short port)
{
    unsigned short ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void outsw(unsigned short port, const void *addr, int count)
{
    __asm__ volatile("rep outsw"
                     : "+S"(addr), "+c"(count)
                     : "d"(port)
                     : "memory");
}

void insw(unsigned short port, void *addr, int count)
{
    __asm__ volatile("rep insw"
                     : "+D"(addr), "+c"(count)
                     : "d"(port)
                     : "memory");
}
