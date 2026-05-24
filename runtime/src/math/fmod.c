#include <math.h>
#include "internal/math_common.h"

double fmod(double x, double y)
{
    if (y == 0.0 || isnan(x) || isnan(y))
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    if (isinf(x) && isfinite(y))
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    if (isinf(y))
    {
        return x;
    }
    double q = x / y;
    double qtr;
    if (q >= 0.0)
        qtr = floor(q);
    else
        qtr = ceil(q);
    double r = x - y * qtr;
    if (fabs(r) >= fabs(y))
    {
        if (r > 0)
            r -= fabs(y);
        else
            r += fabs(y);
    }
    return r;
}
