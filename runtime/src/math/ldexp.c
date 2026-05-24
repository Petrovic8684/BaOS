#include <math.h>
#include "internal/math_common.h"

double ldexp(double x, int exp)
{
    if (x == 0.0 || isnan(x) || isinf(x))
        return x;
    dbl_bits b;
    b.d = x;
    uint32_t hi = b.u32[1];
    uint32_t lo = b.u32[0];
    uint32_t efield = (hi >> 20) & 0x7FFu;

    if (efield == 0)
    {
        if (exp > 1023)
        {
            hi &= ~((uint32_t)0x7FFu << 20);
            hi |= (0x7FFu << 20);
            lo = 0;
            dbl_bits r;
            r.u32[1] = hi;
            r.u32[0] = lo;
            return r.d;
        }
        double res = x;
        if (exp > 0)
        {
            while (exp > 30)
            {
                res *= (1u << 30);
                exp -= 30;
            }
            while (exp > 0)
            {
                res *= 2.0;
                exp--;
            }
        }
        else if (exp < 0)
        {
            while (exp < -30)
            {
                res *= (1.0 / (1u << 30));
                exp += 30;
            }
            while (exp < 0)
            {
                res *= 0.5;
                exp++;
            }
        }
        return res;
    }
    else
    {
        int e = (int)efield + exp;
        if (e <= 0)
        {
            double res = x;
            int ne = exp;
            if (ne < 0)
            {
                while (ne < -30)
                {
                    res *= (1.0 / (1u << 30));
                    ne += 30;
                }
                while (ne < 0)
                {
                    res *= 0.5;
                    ne++;
                }
            }
            else
            {
                while (ne > 30)
                {
                    res *= (1u << 30);
                    ne -= 30;
                }
                while (ne > 0)
                {
                    res *= 2.0;
                    ne--;
                }
            }
            return res;
        }
        else if (e >= 0x7FF)
        {
            hi &= ~((uint32_t)0x7FFu << 20);
            hi |= (0x7FFu << 20);
            lo = 0;
            dbl_bits r;
            r.u32[1] = hi;
            r.u32[0] = lo;
            return r.d;
        }
        else
        {
            hi &= ~((uint32_t)0x7FFu << 20);
            hi |= ((uint32_t)e << 20);
            dbl_bits r;
            r.u32[1] = hi;
            r.u32[0] = lo;
            return r.d;
        }
    }
}
