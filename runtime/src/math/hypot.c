#include <math.h>
#include "internal/math_common.h"

double hypot(double x, double y)
{
    x = fabs(x);
    y = fabs(y);
    if (x < y)
    {
        double t = x;
        x = y;
        y = t;
    }
    if (x == 0.0)
        return 0.0;
    double r = y / x;
    return x * sqrt(1.0 + r * r);
}
