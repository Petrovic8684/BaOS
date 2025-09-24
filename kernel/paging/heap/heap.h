#ifndef HEAP_H
#define HEAP_H

#include "../../info/ram/ram.h"

void heap_init(void);
void *kmalloc(unsigned int size);
void kfree(void *ptr);
void get_heap_info(struct heap_info *info);

#endif
