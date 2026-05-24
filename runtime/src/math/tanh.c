#include <math.h>
#include "internal/math_common.h"

double tanh(double x)
{
    double ex = exp(2.0 * x);
    return (ex - 1.0) / (ex + 1.0);
}
