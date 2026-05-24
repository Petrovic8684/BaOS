#include <math.h>
#include "internal/math_common.h"

double round(double x)
{
    if (!isfinite(x))
        return x;
    double ip;
    double frac = modf(x, &ip);
    if (frac > 0.5)
        return ip + 1.0;
    else if (frac < -0.5)
        return ip - 1.0;
    else if (frac == 0.5)
        return ip + 1.0;
    else if (frac == -0.5)
        return ip - 1.0;
    return ip;
}
