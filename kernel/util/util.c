#include "util.h"

// Convert BCD to binary
unsigned char bcd_to_bin(unsigned char val)
{
    return (val & 0x0F) + ((val >> 4) * 10);
}