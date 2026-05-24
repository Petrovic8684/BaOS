#include <math.h>
#include "internal/math_common.h"

double nan(const char *tagp)
{
    (void)tagp;
    dbl_bits nanp;
    nanp.u32[1] = 0x7FF80000u;
    nanp.u32[0] = 0;
    return nanp.d;
}
