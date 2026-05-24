#include <math.h>
#include "internal/math_common.h"

double cosh(double x)
{
    double ex = exp(x);
    double emx = exp(-x);
    return 0.5 * (ex + emx);
}
