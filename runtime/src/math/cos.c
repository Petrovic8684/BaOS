#include <math.h>
#include "internal/math_common.h"

double cos(double x)
{
    if (!isfinite(x))
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    double r = reduce_periodic(x);
    double x2 = r * r;
    double c = 1.0 - x2 * (1.0 / 2.0) + x2 * x2 * (1.0 / 24.0) - x2 * x2 * x2 * (1.0 / 720.0);
    return c;
}
