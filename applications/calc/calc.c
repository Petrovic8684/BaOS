#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <float.h>

#define INPUT_BUF 256

typedef enum
{
    CALC_OK = 0,
    CALC_ERR_DIV0,
    CALC_ERR_SYNTAX
} CalcStatus;

typedef struct
{
    double value;
    CalcStatus status;
} CalcResult;

static const char *p;
static CalcStatus g_status;

static double var_value[256];

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
    if (isalpha((unsigned char)*p))
    {
        char var = *p;
        p++;
        return var_value[(unsigned char)var];
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

static double newton_raphson(const char *expr, char var, double x0, int max_iter, double tol)
{
    for (int i = 0; i < max_iter; i++)
    {
        var_value[(unsigned char)var] = x0;
        CalcResult res_fx = calc_evaluate(expr);
        double fx = res_fx.value;
        if (res_fx.status != CALC_OK)
        {
            g_status = res_fx.status;
            return 0.0;
        }

        double h = 1e-6;
        var_value[(unsigned char)var] = x0 + h;
        CalcResult res_fxh = calc_evaluate(expr);
        double fxh = res_fxh.value;
        if (res_fxh.status != CALC_OK)
        {
            g_status = res_fxh.status;
            return 0.0;
        }

        double dfx = (fxh - fx) / h;
        if (fabs(dfx) < 1e-12)
        {
            g_status = CALC_ERR_DIV0;
            return 0.0;
        }

        double x1 = x0 - fx / dfx;
        if (fabs(x1 - x0) < tol)
        {
            g_status = CALC_OK;
            return x1;
        }
        x0 = x1;
    }
    g_status = CALC_ERR_SYNTAX;
    return 0.0;
}

static int approx_equal(double a, double b, double tol)
{
    return fabs(a - b) <= tol;
}

static int find_roots(const char *func, char var, double *roots, int max_roots)
{
    double seeds[] = {-1000.0, -100.0, -50.0, -10.0, -5.0, -2.0, -1.0, -0.5, -0.1, 0.0, 0.1, 0.5, 1.0, 2.0, 5.0, 10.0, 50.0, 100.0, 1000.0};
    int nseeds = (int)(sizeof(seeds) / sizeof(seeds[0]));

    for (int i = 0; i < max_roots; i++)
        roots[i] = 0.0;

    int found = 0;
    for (int s = 0; s < nseeds && found < max_roots; s++)
    {
        double seed = seeds[s];

        g_status = CALC_OK;
        double r = newton_raphson(func, var, seed, 200, 1e-9);

        if (g_status != CALC_OK)
            continue;

        if (!isfinite(r) || isnan(r))
            continue;

        bool unique = true;
        for (int i = 0; i < found; i++)
            if (approx_equal(r, roots[i], 1e-7))
            {
                unique = false;
                break;
            }

        if (unique)
        {
            roots[found++] = r;
        }
    }

    return found;
}

static bool solve_equation(const char *expr, char *output)
{
    const char *eq = strchr(expr, '=');
    if (!eq)
        return false;

    char left[INPUT_BUF], right[INPUT_BUF];
    size_t left_len = (size_t)(eq - expr);
    if (left_len >= sizeof(left))
        left_len = sizeof(left) - 1;
    strncpy(left, expr, left_len);
    left[left_len] = '\0';
    strncpy(right, eq + 1, sizeof(right) - 1);
    right[sizeof(right) - 1] = '\0';

    bool seen_var[256] = {0};
    int var_count = 0;
    char var = 0;
    for (const char *q = expr; *q; q++)
        if (isalpha((unsigned char)*q))
        {
            unsigned char ch = (unsigned char)*q;
            if (!seen_var[ch])
            {
                seen_var[ch] = true;
                var_count++;
                var = (char)ch;
            }
        }

    if (var_count == 0)
        return false;

    if (var_count > 1)
        return false;

    char func[2 * INPUT_BUF];
    if (snprintf(func, sizeof(func), "(%s)-(%s)", left, right) >= (int)sizeof(func))
        return false;

    double roots[64];
    int nroots = find_roots(func, var, roots, 64);
    if (nroots == 0)
        return false;

    size_t off = 0;
    int ret = snprintf(output + off, INPUT_BUF - off, "%c=", var);
    if (ret < 0)
        return false;
    off += (size_t)ret;

    for (int i = 0; i < nroots; i++)
    {
        char tmp[64];
        int n = snprintf(tmp, sizeof(tmp), "%g", roots[i]);
        if (n < 0)
            n = 0;
        if (off + (size_t)n + 2 >= INPUT_BUF)
            break;
        if (i > 0)
        {
            output[off++] = ',';
            output[off++] = ' ';
        }
        memcpy(output + off, tmp, (size_t)n);
        off += (size_t)n;
        output[off] = '\0';
    }

    return true;
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

static int read_input(char *buf, int maxlen)
{
    int pos = 0;
    int len = 0;
    buf[0] = '\0';

    while (1)
    {
        int c = getchar();

        if (c == 3)
        {
            if (pos > 0)
            {
                pos--;
                printf("\033[D");
                fflush(stdout);
            }
            continue;
        }
        if (c == 4)
        {
            if (pos < len)
            {
                pos++;
                printf("\033[C");
                fflush(stdout);
            }
            continue;
        }

        if (c == 27)
        {
            buf[0] = '\0';
            return 27;
        }

        if (c == '\r' || c == '\n')
        {
            if (len < maxlen - 1)
            {
                buf[len++] = '\n';
                buf[len] = '\0';
            }
            putchar('\n');
            fflush(stdout);
            return 1;
        }

        if (c == 8)
        {
            if (pos > 0)
            {
                int old_len = len;
                int del_pos = pos - 1;

                for (int i = del_pos; i < len - 1; ++i)
                    buf[i] = buf[i + 1];
                len--;
                buf[len] = '\0';
                pos = del_pos;

                printf("\033[D");

                if (pos < len)
                {
                    fputs(buf + pos, stdout);
                    putchar(' ');
                    int move_back = (len - pos) + 1;
                    if (move_back > 0)
                        printf("\033[%dD", move_back);
                }
                else
                {
                    putchar(' ');
                    printf("\033[D");
                }

                fflush(stdout);
            }
            continue;
        }

        if (c == 127)
        {
            if (pos < len)
            {
                for (int i = pos; i < len - 1; ++i)
                    buf[i] = buf[i + 1];
                len--;
                buf[len] = '\0';

                if (pos < len)
                {
                    fputs(buf + pos, stdout);
                    putchar(' ');
                    int move_back = (len - pos) + 1;
                    if (move_back > 0)
                        printf("\033[%dD", move_back);
                }
                else
                {
                    putchar(' ');
                    printf("\033[%dD", 1);
                }

                fflush(stdout);
            }
            continue;
        }

        if (isalnum((unsigned char)c) || strchr("+-*/.%^()=", c))
        {
            if (len < maxlen - 1)
            {
                for (int i = len; i > pos; --i)
                    buf[i] = buf[i - 1];
                buf[pos] = (char)c;
                len++;
                buf[len] = '\0';

                putchar(c);
                if (pos < len - 1)
                    fputs(buf + pos + 1, stdout);

                int move_back = (len - pos - 1);
                if (move_back > 0)
                    printf("\033[%dD", move_back);

                fflush(stdout);
                pos++;
            }
            continue;
        }
    }
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            char out[INPUT_BUF] = "";
            bool has_letter = strpbrk(argv[i], "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") != NULL;
            bool has_eq = strchr(argv[i], '=') != NULL;

            if (has_letter && has_eq)
            {
                if (!solve_equation(argv[i], out))
                    strcpy(out, "Syntax error");
            }
            else if (!has_letter)
            {
                CalcResult res = calc_evaluate(argv[i]);
                if (res.status == CALC_OK)
                    snprintf(out, sizeof(out), "%s = %g", argv[i], res.value);
                else if (res.status == CALC_ERR_DIV0)
                    snprintf(out, sizeof(out), "%s : Division by zero", argv[i]);
                else
                    snprintf(out, sizeof(out), "%s : Syntax error", argv[i]);
            }
            else
                strcpy(out, "Syntax error");

            printf("%s\n", out);
        }
        return 0;
    }

    printf("\033[2J\033[1;1H");
    printf("\033[30;47m BaOS CALC utility : Press ESC to exit. \033[0K\033[0m\n\n");
    set_scroll_region_big();

    char buf[INPUT_BUF];
    char last_result[INPUT_BUF] = "";

    while (1)
    {
        printf("\033[2;1H\033[J");
        printf("\033[5;1H");

        if (last_result[0] != '\0')
            printf("%s\n\n", last_result);

        printf("\033[3;1H\033[2K");
        printf("> ");
        fflush(stdout);

        int r = read_input(buf, 78);

        if (r == 27)
        {
            printf("\033[2J");
            break;
        }

        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n')
            buf[len - 1] = '\0';

        if (buf[0] == '\0')
            continue;

        for (int i = 0; i < 256; i++)
            var_value[i] = 0.0;

        last_result[0] = '\0';
        bool has_letter = strpbrk(buf, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") != NULL;
        bool has_eq = strchr(buf, '=') != NULL;

        if (has_letter && has_eq)
        {
            if (!solve_equation(buf, last_result))
                strcpy(last_result, "Syntax error");
        }
        else if (!has_letter)
        {
            CalcResult res = calc_evaluate(buf);
            if (res.status == CALC_OK)
                snprintf(last_result, sizeof(last_result), "%s = %g", buf, res.value);
            else if (res.status == CALC_ERR_DIV0)
                snprintf(last_result, sizeof(last_result), "%s : Division by zero", buf);
            else
                snprintf(last_result, sizeof(last_result), "%s : Syntax error", buf);
        }
        else
            strcpy(last_result, "Syntax error");

        printf("%s\n", last_result);
        fflush(stdout);
    }

    reset_scroll_region();
    fflush(stdout);
    return 0;
}