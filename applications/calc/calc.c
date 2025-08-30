#include "calc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>

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

static int is_integer(double x)
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

static CalcResult calc_evaluate(const char *expr)
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

static void set_scroll_region_big(void)
{
    printf("\033[3;9999r");
    fflush(stdout);
}

static void reset_scroll_region(void)
{
    printf("\033[r");
    fflush(stdout);
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
                printf("\033[31m%s : Division by zero.\033[0m\n", argv[i]);
            else
                printf("\033[31m%s : Syntax error.\033[0m\n", argv[i]);
        }
        return 0;
    }

    printf("\033[2J\033[1;1H");
    printf("\033[30;47m BaOS CALC utility : Press ESC to exit. \033[0K\033[0m\n\n");

    set_scroll_region_big();

    char buf[INPUT_BUF];
    char last_result[128] = "";

    while (1)
    {
        printf("\033[2;1H\033[J");
        printf("\033[5;1H");

        if (last_result[0] != '\0')
            printf("%s\n\n", last_result);

        printf("\033[3;1H\033[2K");
        printf("> ");
        fflush(stdout);

        read_line(buf, INPUT_BUF);

        if (buf_has_esc(buf, INPUT_BUF))
        {
            printf("\033[2J");
            break;
        }

        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n')
            buf[len - 1] = '\0';

        if (buf[0] == '\0')
            continue;

        CalcResult res = calc_evaluate(buf);
        last_result[0] = '\0';

        int i = 0;

        for (size_t j = 0; j < strlen(buf); j++)
            last_result[i++] = buf[j];

        last_result[i++] = ' ';

        if (res.status == CALC_OK)
        {
            last_result[i++] = '=';
            last_result[i++] = ' ';

            double val = res.value;
            if (val < 0)
            {
                last_result[i++] = '-';
                val = -val;
            }

            int whole = (int)val;
            double frac = val - whole;

            if (whole == 0)
                last_result[i++] = '0';
            else
            {
                char tmp[32];
                int j = 0;
                int temp = whole;
                while (temp > 0)
                {
                    tmp[j++] = '0' + (temp % 10);
                    temp /= 10;
                }
                for (int k = j - 1; k >= 0; k--)
                    last_result[i++] = tmp[k];
            }

            last_result[i++] = '.';

            for (int d = 0; d < 10; d++)
            {
                frac *= 10;
                int digit = (int)frac;
                last_result[i++] = '0' + digit;
                frac -= digit;
            }
        }
        else if (res.status == CALC_ERR_DIV0)
        {
            last_result[i++] = ':';
            last_result[i++] = ' ';
            const char *msg = "\033[31mDivision by zero.\033[0m";
            for (size_t j = 0; j < strlen(msg); j++)
                last_result[i++] = msg[j];
        }
        else
        {
            last_result[i++] = ':';
            last_result[i++] = ' ';
            const char *msg = "\033[31mSyntax error.\033[0m";
            for (size_t j = 0; j < strlen(msg); j++)
                last_result[i++] = msg[j];
        }

        last_result[i] = '\0';
    }

    reset_scroll_region();
    fflush(stdout);

    return 0;
}