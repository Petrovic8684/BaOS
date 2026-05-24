#include <math.h>
#include "internal/math_common.h"

double atan(double x)
{
    if (!isfinite(x))
        return (isnan(x) ? x : (x > 0 ? 3.14159265358979323846 / 2.0 : -3.14159265358979323846 / 2.0));
    int sign = 0;
    if (x < 0)
    {
        x = -x;
        sign = 1;
    }
    double res;
    if (x <= 1.0)
    {
        double x2 = x * x;
        res = x * (1.0 - x2 * (1.0 / 3.0) + x2 * x2 * (1.0 / 5.0));
    }
    else
    {
        double y = 1.0 / x;
        double y2 = y * y;
        double t = y * (1.0 - y2 * (1.0 / 3.0) + y2 * y2 * (1.0 / 5.0));
        res = 1.57079632679489661923 - t;
    }
    if (sign)
        res = -res;
    return res;
}
