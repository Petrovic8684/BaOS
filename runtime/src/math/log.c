#include <math.h>
#include "internal/math_common.h"

double log(double x)
{
    if (x < 0.0)
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    if (x == 0.0)
    {
        dbl_bits ninf;
        ninf.u32[1] = 0xFFF00000u;
        ninf.u32[0] = 0u;
        return ninf.d;
    }
    if (isnan(x) || isinf(x))
        return x;
    int e;
    double m = frexp(x, &e);
    const double ln2 = 0.693147180559945309417232121458176568;
    double y = (m - 1.0) / (m + 1.0);
    double y2 = y * y;
    double term = y;
    double sum = term;
    for (int k = 3; k <= 15; k += 2)
    {
        term *= y2;
        sum += term / (double)k;
    }
    double lnm = 2.0 * sum;
    return lnm + (double)(e - 0) * ln2;
}
