#include <math.h>
#include "internal/math_common.h"

double pow(double x, double y)
{
    if (y == 0.0)
        return 1.0;
    if (x == 1.0)
        return 1.0;
    if (isnan(x) || isnan(y))
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    if (x < 0.0)
    {
        int yi;
        if (is_integer_double(y, &yi))
        {
            double r = exp(y * log(-x));
            if (yi & 1)
                return -r;
            return r;
        }
        else
        {
            dbl_bits nanp;
            nanp.u32[1] = 0x7FF80000u;
            nanp.u32[0] = 0u;
            return nanp.d;
        }
    }
    return exp(y * log(x));
}
