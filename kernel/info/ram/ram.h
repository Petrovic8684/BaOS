#ifndef RAM_H
#define RAM_H

struct heap_info
{
    unsigned int heap_start;
    unsigned int heap_end;
    unsigned int heap_max;
    unsigned int free_bytes;
};

#endif
