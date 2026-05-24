#include <math.h>
#include "internal/math_common.h"

int isfinite(double x)
{
    dbl_bits b;
    b.d = x;
    uint32_t e = (b.u32[1] >> 20) & 0x7FFu;
    return e != 0x7FFu;
}
