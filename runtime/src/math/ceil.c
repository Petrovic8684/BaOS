#include <math.h>
#include "internal/math_common.h"

double ceil(double x)
{
    if (!isfinite(x))
        return x;
    double ip;
    double frac = modf(x, &ip);
    if (x <= 0.0)
        return ip;
    else
    {
        if (frac == 0.0)
            return ip;
        return ip + 1.0;
    }
}
