#include <math.h>
#include "internal/math_common.h"

double asin(double x)
{
    if (x > 1.0 || x < -1.0)
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    return atan(x / sqrt(1.0 - x * x));
}
