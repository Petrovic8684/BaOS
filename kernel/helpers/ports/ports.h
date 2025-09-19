#ifndef PORTS_H
#define PORTS_H

void outb(unsigned short port, unsigned char val);
unsigned char inb(unsigned short port);

void outw(unsigned short port, unsigned short val);
unsigned short inw(unsigned short port);

void outsw(unsigned short port, const void *addr, int count);
void insw(unsigned short port, void *addr, int count);

#endif