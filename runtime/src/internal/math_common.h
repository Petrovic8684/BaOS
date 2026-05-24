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

int is_integer_double(double x, int *ival);
double reduce_periodic(double x);
