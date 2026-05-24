#include <math.h>
#include "internal/math_common.h"

double tan(double x)
{
    double c = cos(x);
    if (c == 0.0)
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    return sin(x) / c;
}
