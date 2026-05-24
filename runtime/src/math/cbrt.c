#include <math.h>
#include "internal/math_common.h"

double cbrt(double x)
{
    if (x == 0.0)
        return 0.0;
    double s = (x > 0.0) ? 1.0 : -1.0;
    x = fabs(x);
    double y = exp(log(x) / 3.0);
    return s * y;
}
