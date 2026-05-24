#include <math.h>
#include "internal/math_common.h"

double frexp(double x, int *exp)
{
    dbl_bits b;
    b.d = x;
    uint32_t hi = b.u32[1];
    uint32_t lo = b.u32[0];
    uint32_t sign = hi & DBL_SIGN_MASK_HIGH;
    uint32_t efield = (hi >> 20) & 0x7FFu;

    if (efield == 0)
    {
        if ((hi & ~DBL_SIGN_MASK_HIGH) == 0 && lo == 0)
        {
            if (exp)
                *exp = 0;
            return x;
        }

        const double two53 = 9007199254740992.0;
        double xs = x * two53;
        dbl_bits bs;
        bs.d = xs;
        uint32_t hi2 = bs.u32[1];
        uint32_t efield2 = (hi2 >> 20) & 0x7FFu;
        int e = (int)efield2 - DBL_EXP_BIAS;
        hi2 &= ~((uint32_t)0x7FFu << 20);
        hi2 |= ((DBL_EXP_BIAS - 1) << 20);
        bs.u32[1] = hi2;
        double m = bs.d;
        if (exp)
            *exp = e - 53 + 1;
        dbl_bits bm;
        bm.d = m;
        bm.u32[1] |= sign;
        return bm.d;
    }
    else if (efield == 0x7FFu)
    {
        if (exp)
            *exp = 0;
        return x;
    }
    else
    {
        int e = (int)efield - (DBL_EXP_BIAS - 1);
        hi &= ~((uint32_t)0x7FFu << 20);
        hi |= ((DBL_EXP_BIAS - 1) << 20);
        dbl_bits bm;
        bm.u32[1] = hi;
        bm.u32[0] = lo;
        double m = bm.d;
        if (exp)
            *exp = e;
        return m;
    }
}
