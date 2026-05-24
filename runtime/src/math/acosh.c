#include <math.h>
#include "internal/math_common.h"

double acosh(double x)
{
    if (x < 1.0)
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0;
        return nanp.d;
    }
    return log(x + sqrt(x * x - 1.0));
}
