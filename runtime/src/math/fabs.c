#include <math.h>
#include "internal/math_common.h"

double fabs(double x)
{
    dbl_bits b;
    b.d = x;
    b.u32[1] &= ~DBL_SIGN_MASK_HIGH;
    return b.d;
}
