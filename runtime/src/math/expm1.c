#include <math.h>
#include "internal/math_common.h"

double expm1(double x)
{
    if (fabs(x) < 1e-5)
        return x + 0.5 * x * x;
    return exp(x) - 1.0;
}
