#include <math.h>
#include "internal/math_common.h"

double modf(double x, double *intpart)
{
    if (intpart == NULL)
        return x;
    if (!isfinite(x))
    {
        if (isnan(x))
        {
            *intpart = x;
            return x;
        }
        else
        {
            *intpart = x;
            return 0.0 * x;
        }
    }
    dbl_bits b;
    b.d = x;
    uint32_t hi = b.u32[1];
    uint32_t lo = b.u32[0];
    uint32_t sign = hi & DBL_SIGN_MASK_HIGH;
    uint32_t efield = (hi >> 20) & 0x7FFu;

    if (efield == 0)
    {
        *intpart = set_words(sign, 0u);
        return x;
    }

    int e = (int)efield - DBL_EXP_BIAS;
    if (e < 0)
    {
        *intpart = set_words(sign, 0u);
        return x;
    }
    else if (e >= 52)
    {
        *intpart = x;
        return set_words(sign, 0u);
    }
    else
    {
        int frac_bits = 52 - e;
        uint32_t mask_hi = 0;
        uint32_t mask_lo = 0;
        if (frac_bits >= 32)
        {
            int hb = frac_bits - 32;
            if (hb >= 20)
                mask_hi = 0;
            else
                mask_hi = ~((1u << hb) - 1u) & DBL_FRAC_MASK_HIGH;

            mask_lo = 0u;
        }
        else
        {
            if (frac_bits == 0)
                mask_lo = 0xFFFFFFFFu;
            else
                mask_lo = ~((1u << frac_bits) - 1u);
            mask_hi = DBL_FRAC_MASK_HIGH;
        }
        uint32_t new_hi = (hi & (DBL_SIGN_MASK_HIGH | (0x7FFu << 20) | mask_hi));
        uint32_t new_lo = lo & mask_lo;
        dbl_bits intb;
        intb.u32[1] = new_hi;
        intb.u32[0] = new_lo;
        double ip = intb.d;
        *intpart = ip;
        double frac = x - ip;
        return frac;
    }
}
