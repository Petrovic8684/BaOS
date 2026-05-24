#include <math.h>
#include "internal/math_common.h"

double sqrt(double x)
{
    if (x < 0.0)
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    if (x == 0.0 || isinf(x) || isnan(x))
        return x;
    dbl_bits b;
    b.d = x;
    uint32_t hi = b.u32[1];
    uint32_t lo = b.u32[0];
    uint32_t efield = (hi >> 20) & 0x7FFu;
    double y;
    if (efield == 0)
    {
        y = x * (double)(1ULL << 20);
        y = sqrt(y) * (1.0 / (1ULL << 10));
    }
    else
    {
        int e = (int)efield;
        int newexp = ((e - DBL_EXP_BIAS) >> 1) + DBL_EXP_BIAS;
        hi &= ~((uint32_t)0x7FFu << 20);
        hi |= ((uint32_t)newexp << 20);
        dbl_bits approx;
        approx.u32[1] = hi;
        approx.u32[0] = lo;
        y = approx.d;
    }
    for (int i = 0; i < 6; ++i)
    {
        double y_old = y;
        y = 0.5 * (y + x / y);
        if (y == y_old)
            break;
    }
    return y;
}
