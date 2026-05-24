#include <math.h>
#include "internal/math_common.h"

double trunc(double x)
{
    double ip;
    modf(x, &ip);
    return ip;
}
