#include <math.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

typedef union
{
    double d;
    uint32_t u32[2];
} dbl_bits;

static inline uint32_t high_word(double x)
{
    dbl_bits b;
    b.d = x;
    return b.u32[1];
}

static inline uint32_t low_word(double x)
{
    dbl_bits b;
    b.d = x;
    return b.u32[0];
}

static inline double set_words(uint32_t hi, uint32_t lo)
{
    dbl_bits b;
    b.u32[1] = hi;
    b.u32[0] = lo;
    return b.d;
}

#define DBL_EXP_MASK_HIGH 0x7FF00000u
#define DBL_SIGN_MASK_HIGH 0x80000000u
#define DBL_FRAC_MASK_HIGH 0x000FFFFFu
#define DBL_EXP_BIAS 1023

int isnan(double x)
{
    dbl_bits b;
    b.d = x;
    uint32_t e = (b.u32[1] >> 20) & 0x7FFu;
    uint32_t frac_hi = b.u32[1] & DBL_FRAC_MASK_HIGH;
    uint32_t frac_lo = b.u32[0];
    return (e == 0x7FFu) && (frac_hi != 0 || frac_lo != 0);
}

int isinf(double x)
{
    dbl_bits b;
    b.d = x;
    uint32_t e = (b.u32[1] >> 20) & 0x7FFu;
    uint32_t frac_hi = b.u32[1] & DBL_FRAC_MASK_HIGH;
    uint32_t frac_lo = b.u32[0];
    return (e == 0x7FFu) && (frac_hi == 0 && frac_lo == 0);
}

int isfinite(double x)
{
    dbl_bits b;
    b.d = x;
    uint32_t e = (b.u32[1] >> 20) & 0x7FFu;
    return e != 0x7FFu;
}

double fabs(double x)
{
    dbl_bits b;
    b.d = x;
    b.u32[1] &= ~DBL_SIGN_MASK_HIGH;
    return b.d;
}

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

double floor(double x)
{
    if (!isfinite(x))
        return x;
    double ip;
    double frac = modf(x, &ip);
    if (x >= 0.0)
        return ip;
    else
    {
        if (frac == 0.0)
            return ip;
        return ip - 1.0;
    }
}
double ceil(double x)
{
    if (!isfinite(x))
        return x;
    double ip;
    double frac = modf(x, &ip);
    if (x <= 0.0)
        return ip;
    else
    {
        if (frac == 0.0)
            return ip;
        return ip + 1.0;
    }
}

double sqrt(double x)
{
    if (x < 0.0)
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    if (x == 0.0 || isinf(x) || isnan(x))
        return x;
    dbl_bits b;
    b.d = x;
    uint32_t hi = b.u32[1];
    uint32_t lo = b.u32[0];
    uint32_t efield = (hi >> 20) & 0x7FFu;
    double y;
    if (efield == 0)
    {
        y = x * (double)(1ULL << 20);
        y = sqrt(y) * (1.0 / (1ULL << 10));
    }
    else
    {
        int e = (int)efield;
        int newexp = ((e - DBL_EXP_BIAS) >> 1) + DBL_EXP_BIAS;
        hi &= ~((uint32_t)0x7FFu << 20);
        hi |= ((uint32_t)newexp << 20);
        dbl_bits approx;
        approx.u32[1] = hi;
        approx.u32[0] = lo;
        y = approx.d;
    }
    for (int i = 0; i < 6; ++i)
    {
        double y_old = y;
        y = 0.5 * (y + x / y);
        if (y == y_old)
            break;
    }
    return y;
}

double fmod(double x, double y)
{
    if (y == 0.0 || isnan(x) || isnan(y))
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    if (isinf(x) && isfinite(y))
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    if (isinf(y))
    {
        return x;
    }
    double q = x / y;
    double qtr;
    if (q >= 0.0)
        qtr = floor(q);
    else
        qtr = ceil(q);
    double r = x - y * qtr;
    if (fabs(r) >= fabs(y))
    {
        if (r > 0)
            r -= fabs(y);
        else
            r += fabs(y);
    }
    return r;
}

double log(double x);
double exp(double x);

static int is_integer_double(double x, int *ival)
{
    if (!isfinite(x))
        return 0;
    double ip;
    double frac = modf(x, &ip);
    if (frac == 0.0 && ip >= (double)INT_MIN && ip <= (double)INT_MAX)
    {
        if (ival)
            *ival = (int)ip;
        return 1;
    }
    return 0;
}

double pow(double x, double y)
{
    if (y == 0.0)
        return 1.0;
    if (x == 1.0)
        return 1.0;
    if (isnan(x) || isnan(y))
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    if (x < 0.0)
    {
        int yi;
        if (is_integer_double(y, &yi))
        {
            double r = exp(y * log(-x));
            if (yi & 1)
                return -r;
            return r;
        }
        else
        {
            dbl_bits nanp;
            nanp.u32[1] = 0x7FF80000u;
            nanp.u32[0] = 0u;
            return nanp.d;
        }
    }
    return exp(y * log(x));
}

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

double log10(double x)
{
    static const double inv_ln10 = 0.434294481903251827651;
    return log(x) * inv_ln10;
}

static double reduce_periodic(double x)
{
    const double TWO_PI = 6.283185307179586476925286766559;
    if (!isfinite(x))
        return x;
    double r = fmod(x, TWO_PI);
    if (r > 3.14159265358979323846)
        r -= TWO_PI;
    else if (r < -3.14159265358979323846)
        r += TWO_PI;
    return r;
}

double sin(double x)
{
    if (!isfinite(x))
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    double r = reduce_periodic(x);
    double x2 = r * r;
    double s = r * (1.0 - x2 * (1.0 / 6.0) + x2 * x2 * (1.0 / 120.0) - x2 * x2 * x2 * (1.0 / 5040.0));
    return s;
}

double cos(double x)
{
    if (!isfinite(x))
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    double r = reduce_periodic(x);
    double x2 = r * r;
    double c = 1.0 - x2 * (1.0 / 2.0) + x2 * x2 * (1.0 / 24.0) - x2 * x2 * x2 * (1.0 / 720.0);
    return c;
}

double tan(double x)
{
    double c = cos(x);
    if (c == 0.0)
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    return sin(x) / c;
}

double atan(double x)
{
    if (!isfinite(x))
        return (isnan(x) ? x : (x > 0 ? 3.14159265358979323846 / 2.0 : -3.14159265358979323846 / 2.0));
    int sign = 0;
    if (x < 0)
    {
        x = -x;
        sign = 1;
    }
    double res;
    if (x <= 1.0)
    {
        double x2 = x * x;
        res = x * (1.0 - x2 * (1.0 / 3.0) + x2 * x2 * (1.0 / 5.0));
    }
    else
    {
        double y = 1.0 / x;
        double y2 = y * y;
        double t = y * (1.0 - y2 * (1.0 / 3.0) + y2 * y2 * (1.0 / 5.0));
        res = 1.57079632679489661923 - t;
    }
    if (sign)
        res = -res;
    return res;
}

double atan2(double y, double x)
{
    if (isnan(x) || isnan(y))
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    if (x > 0)
        return atan(y / x);
    if (x < 0 && y >= 0)
        return atan(y / x) + 3.14159265358979323846;
    if (x < 0 && y < 0)
        return atan(y / x) - 3.14159265358979323846;
    if (x == 0 && y > 0)
        return 1.57079632679489661923;
    if (x == 0 && y < 0)
        return -1.57079632679489661923;
    return 0.0;
}

double asin(double x)
{
    if (x > 1.0 || x < -1.0)
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    return atan(x / sqrt(1.0 - x * x));
}

double acos(double x)
{
    if (x > 1.0 || x < -1.0)
    {
        dbl_bits nanp;
        nanp.u32[1] = 0x7FF80000u;
        nanp.u32[0] = 0u;
        return nanp.d;
    }
    return 1.57079632679489661923 - asin(x);
}

double sinh(double x)
{
    double ex = exp(x);
    double emx = exp(-x);
    return 0.5 * (ex - emx);
}

double cosh(double x)
{
    double ex = exp(x);
    double emx = exp(-x);
    return 0.5 * (ex + emx);
}

double tanh(double x)
{
    double ex = exp(2.0 * x);
    return (ex - 1.0) / (ex + 1.0);
}

double hypot(double x, double y)
{
    x = fabs(x);
    y = fabs(y);
    if (x < y)
    {
        double t = x;
        x = y;
        y = t;
    }
    if (x == 0.0)
        return 0.0;
    double r = y / x;
    return x * sqrt(1.0 + r * r);
}
