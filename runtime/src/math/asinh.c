#include <math.h>
#include "internal/math_common.h"

double asinh(double x)
{
    if (x == 0.0)
        return x;
    double s = (x > 0) ? 1.0 : -1.0;
    x = fabs(x);
    return s * log(x + sqrt(x * x + 1.0));
}
