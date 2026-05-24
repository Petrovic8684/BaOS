#include <math.h>
#include "internal/math_common.h"

double exp(double x)
{
    if (isnan(x))
        return x;
    if (x > 709.782712893384)
    {
        dbl_bits inf;
        inf.u32[1] = 0x7FF00000u;
        inf.u32[0] = 0u;
        return inf.d;
    }
    if (x < -745.1332191019411)
    {
        return 0.0;
    }
    const double ln2 = 0.693147180559945309417232121458176568;
    double n_d = floor(x / ln2 + 0.5);
    int n = (int)n_d;
    double r = x - n_d * ln2;

    double r2 = r * r;
    double term = 1.0 + r * (1.0 + r * (0.5 + r * (1.0 / 6.0 + r * (1.0 / 24.0 + r * (1.0 / 120.0 + r * (1.0 / 720.0))))));
    double res = term;
    res = ldexp(res, n);
    return res;
}
