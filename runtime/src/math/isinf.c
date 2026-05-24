#include <math.h>
#include "internal/math_common.h"

int isinf(double x)
{
    dbl_bits b;
    b.d = x;
    uint32_t e = (b.u32[1] >> 20) & 0x7FFu;
    uint32_t frac_hi = b.u32[1] & DBL_FRAC_MASK_HIGH;
    uint32_t frac_lo = b.u32[0];
    return (e == 0x7FFu) && (frac_hi == 0 && frac_lo == 0);
}
