#include <stdlib.h>

lldiv_t lldiv(long long numer, long long denom)
{
    lldiv_t r;
    int neg_q = 0, neg_r = 0;

    if (numer < 0)
    {
        numer = -numer;
        neg_q = 1;
    }
    if (denom < 0)
    {
        denom = -denom;
        neg_q ^= 1;
    }

    r.quot = 0;
    r.rem = numer;

    while (r.rem >= denom)
    {
        r.rem -= denom;
        r.quot++;
    }

    if (neg_q)
        r.quot = -r.quot;
    if ((numer < 0) != (denom < 0))
        r.rem = -r.rem;

    return r;
}
