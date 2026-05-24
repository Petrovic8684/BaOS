#include "internal/math_common.h"



int is_integer_double(double x, int *ival)
{
    if (!isfinite(x))
        return 0;
    double ip;
    double frac = modf(x, &ip);
    if (frac == 0.0 && ip >= (double)INT_MIN && ip <= (double)INT_MAX)
    {
        if (ival)
            *ival = (int)ip;
        return 1;
    }
    return 0;
}



double reduce_periodic(double x)
{
    const double TWO_PI = 6.283185307179586476925286766559;
    if (!isfinite(x))
        return x;
    double r = fmod(x, TWO_PI);
    if (r > 3.14159265358979323846)
        r -= TWO_PI;
    else if (r < -3.14159265358979323846)
        r += TWO_PI;
    return r;
}
