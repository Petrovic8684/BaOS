#include <math.h>
#include "internal/math_common.h"

double sinh(double x)
{
    double ex = exp(x);
    double emx = exp(-x);
    return 0.5 * (ex - emx);
}
