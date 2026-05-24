#include <math.h>
#include "internal/math_common.h"

double log1p(double x)
{
    if (x <= -1.0)
    {
        dbl_bits ninf;
        ninf.u32[1] = 0xFFF00000u;
        ninf.u32[0] = 0;
        return ninf.d;
    }
    if (fabs(x) < 1e-8)
        return x - 0.5 * x * x;
    return log(1.0 + x);
}
