#include "calc.h"

#define INPUT_BUF 256

static const char *p;
static CalcStatus g_status;

static double parse_expr(void);
static double parse_term(void);
static double parse_power(void);
static double parse_factor(void);
static double parse_primary(void);

static void skip_spaces(void)
{
    while (*p && isspace((unsigned char)*p))
        p++;
}

int is_integer(double x)
{
    double intpart;
    double frac = modf(fabs(x), &intpart);
    return frac < 1e-9;
}

static double parse_number(void)
{
    skip_spaces();
    char *endptr = NULL;
    double val = strtod(p, &endptr);
    if (endptr == p)
    {
        g_status = CALC_ERR_SYNTAX;
        return 0.0;
    }
    p = endptr;
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
    double left = parse_factor();
    skip_spaces();
    if (*p == '^')
    {
        p++;
        double right = parse_power();
        left = pow(left, right);
    }
    return left;
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
            if (fabs(rhs) < 1e-12)
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
            if (!is_integer(val) || !is_integer(rhs))
            {
                g_status = CALC_ERR_SYNTAX;
                return 0.0;
            }
            long a = (long)val;
            long b = (long)rhs;
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

CalcResult calc_evaluate(const char *expr)
{
    CalcResult res;
    if (!expr)
    {
        res.value = 0.0;
        res.status = CALC_ERR_SYNTAX;
        return res;
    }

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

static bool buf_has_esc(const char *buf, int len)
{
    for (int i = 0; i < len; i++)
        if (buf[i] == 27)
            return true;

    return false;
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            CalcResult res = calc_evaluate(argv[i]);
            if (res.status == CALC_OK)
                printf("%s = %g\n", argv[i], res.value);
            else if (res.status == CALC_ERR_DIV0)
                printf("%s : Division by zero.\n", argv[i]);
            else
                printf("%s : Syntax error.\n", argv[i]);
        }
        return 0;
    }

    printf("CALC utility: Press ESC to exit.\n");

    char buf[INPUT_BUF];
    while (1)
    {
        printf("\n> ");
        fflush(stdout);

        read_line(buf, INPUT_BUF);

        if (buf_has_esc(buf, INPUT_BUF))
            break;

        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n')
            buf[len - 1] = '\0';

        if (buf[0] == '\0')
            continue;

        CalcResult res = calc_evaluate(buf);
        if (res.status == CALC_OK)
            printf("\n%g\n", res.value);
        else if (res.status == CALC_ERR_DIV0)
            printf("\nDivision by zero.\n");
        else
            printf("\nSyntax error.\n");
    }

    return 0;
}