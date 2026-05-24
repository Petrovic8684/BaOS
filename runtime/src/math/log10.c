#include <math.h>
#include "internal/math_common.h"

double log10(double x)
{
    static const double inv_ln10 = 0.434294481903251827651;
    return log(x) * inv_ln10;
}
