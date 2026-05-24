#include <math.h>
#include "internal/math_common.h"

double inf(int sign)
{
    dbl_bits infp;
    infp.u32[1] = 0x7FF00000u | (sign < 0 ? 0x80000000u : 0);
    infp.u32[0] = 0;
    return infp.d;
}
