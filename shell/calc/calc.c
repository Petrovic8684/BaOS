#include "calc.h"

static const char *p;
static CalcStatus g_status;

static double parse_expr(void);
static double parse_term(void);
static double parse_power(void);
static double parse_factor(void);
static double parse_primary(void);

static inline int is_digit(char ch) { return ch >= '0' && ch <= '9'; }
static inline int is_space(char ch) { return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'; }

static void skip_spaces(void)
{
    while (is_space(*p))
        p++;
}

static double dabs(double x) { return x < 0 ? -x : x; }

static int is_int_like(double x)
{
    int t = (int)x;
    return dabs(x - (double)t) < 1e-9;
}

static double pow_custom(double base, double exp)
{
    if (exp == 0.0)
        return 1.0;
    if (base == 0.0)
        return 0.0;

    int neg = 0;
    if (exp < 0)
    {
        neg = 1;
        exp = -exp;
    }

    long long intpart = (long long)exp;
    double frac = exp - (double)intpart;

    double result = 1.0;
    for (long long i = 0; i < intpart; i++)
        result *= base;

    if (frac > 1e-12)
    {
        double guess = 1.0;
        for (int i = 0; i < 20; i++)
            guess = guess - (pow_custom(guess, 1.0 / frac) - base) / (1.0 / (frac)*pow_custom(guess, (1.0 / frac) - 1.0));
        result *= guess;
    }

    if (neg)
        return 1.0 / result;
    return result;
}

static double parse_number(void)
{
    int intpart = 0;
    int has_digits = 0;

    while (is_digit(*p))
    {
        has_digits = 1;
        intpart = intpart * 10 + (*p - '0');
        p++;
    }

    double val = (double)intpart;

    if (*p == '.')
    {
        p++;
        int frac = 0;
        int scale = 1;
        int has_frac = 0;

        while (is_digit(*p))
        {
            has_frac = 1;
            frac = frac * 10 + (*p - '0');
            scale *= 10;
            p++;
        }

        if (!has_digits && !has_frac)
        {
            g_status = CALC_ERR_SYNTAX;
            return 0.0;
        }

        val += (double)frac / (double)scale;
    }
    else if (!has_digits)
    {
        g_status = CALC_ERR_SYNTAX;
        return 0.0;
    }

    return val;
}

static double parse_primary(void)
{
    skip_spaces();
    if (*p == '(')
    {
        p++;
        double v = parse_expr();
        skip_spaces();
        if (*p == ')')
            p++;
        else
            g_status = CALC_ERR_SYNTAX;

        return v;
    }

    return parse_number();
}

static double parse_factor(void)
{
    skip_spaces();
    if (*p == '+')
    {
        p++;
        return parse_factor();
    }
    if (*p == '-')
    {
        p++;
        return -parse_factor();
    }
    return parse_primary();
}

static double parse_power(void)
{
    double val = parse_factor();
    skip_spaces();
    if (*p == '^')
    {
        p++;
        double rhs = parse_power();
        val = pow_custom(val, rhs);
    }
    return val;
}

static double parse_term(void)
{
    double val = parse_power();
    for (;;)
    {
        skip_spaces();
        if (*p == '*')
        {
            p++;
            val *= parse_power();
        }
        else if (*p == '/')
        {
            p++;
            double rhs = parse_power();
            if (dabs(rhs) < 1e-12)
            {
                g_status = CALC_ERR_DIV0;
                return 0.0;
            }
            val /= rhs;
        }
        else if (*p == '%')
        {
            p++;
            double rhs = parse_power();
            if (!is_int_like(val) || !is_int_like(rhs))
            {
                g_status = CALC_ERR_SYNTAX;
                return 0.0;
            }
            int a = (int)val;
            int b = (int)rhs;
            if (b == 0)
            {
                g_status = CALC_ERR_DIV0;
                return 0.0;
            }
            val = (double)(a % b);
        }
        else
            break;
    }
    return val;
}

static double parse_expr(void)
{
    double val = parse_term();
    for (;;)
    {
        skip_spaces();
        if (*p == '+')
        {
            p++;
            val += parse_term();
        }
        else if (*p == '-')
        {
            p++;
            val -= parse_term();
        }
        else
            break;
    }
    return val;
}

int is_integer(double x)
{
    return (x > 0 ? x : -x) - (int)(x > 0 ? x : -x) < 1e-9;
}

CalcResult calc_evaluate(const char *expr)
{
    CalcResult res;
    g_status = CALC_OK;
    p = expr;

    double v = parse_expr();
    skip_spaces();

    if (*p != '\0' && g_status == CALC_OK)
        g_status = CALC_ERR_SYNTAX;

    res.value = (g_status == CALC_OK) ? v : 0.0;
    res.status = g_status;
    return res;
}
