#include <math.h>
#include "internal/math_common.h"

double atan2(double y, double x)
{
    if (isnan(x) || isnan(y))
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    if (x > 0)
        return atan(y / x);
    if (x < 0 && y >= 0)
        return atan(y / x) + 3.14159265358979323846;
    if (x < 0 && y < 0)
        return atan(y / x) - 3.14159265358979323846;
    if (x == 0 && y > 0)
        return 1.57079632679489661923;
    if (x == 0 && y < 0)
        return -1.57079632679489661923;
    return 0.0;
}
