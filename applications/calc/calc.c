#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <float.h>

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

static double *var_value = NULL;

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

static int find_roots(const char *func, char var, double **roots_ptr, int *found_count)
{
    double seeds[] = {-1000.0, -100.0, -50.0, -10.0, -5.0, -2.0, -1.0, -0.5, -0.1,
                      0.0, 0.1, 0.5, 1.0, 2.0, 5.0, 10.0, 50.0, 100.0, 1000.0};
    int nseeds = (int)(sizeof(seeds) / sizeof(seeds[0]));

    *roots_ptr = NULL;
    *found_count = 0;
    int capacity = 8;
    *roots_ptr = malloc(capacity * sizeof(double));
    if (!*roots_ptr)
        return 0;

    for (int s = 0; s < nseeds; s++)
    {
        double seed = seeds[s];
        g_status = CALC_OK;
        double r = newton_raphson(func, var, seed, 200, 1e-9);
        if (g_status != CALC_OK)
            continue;
        if (!isfinite(r) || isnan(r))
            continue;

        bool unique = true;
        for (int i = 0; i < *found_count; i++)
        {
            if (approx_equal(r, (*roots_ptr)[i], 1e-7))
            {
                unique = false;
                break;
            }
        }

        if (unique)
        {
            if (*found_count >= capacity)
            {
                capacity *= 2;
                double *tmp = realloc(*roots_ptr, capacity * sizeof(double));
                if (!tmp)
                {
                    free(*roots_ptr);
                    *roots_ptr = NULL;
                    *found_count = 0;
                    return 0;
                }
                *roots_ptr = tmp;
            }
            (*roots_ptr)[(*found_count)++] = r;
        }
    }

    return *found_count;
}

static bool solve_equation(const char *expr, char **output_ptr)
{
    const char *eq = strchr(expr, '=');
    if (!eq)
        return false;

    size_t left_len = (size_t)(eq - expr);
    size_t right_len = strlen(eq + 1);

    char *left = malloc(left_len + 1);
    char *right = malloc(right_len + 1);
    if (!left || !right)
    {
        free(left);
        free(right);
        return false;
    }
    strncpy(left, expr, left_len);
    left[left_len] = '\0';
    strncpy(right, eq + 1, right_len);
    right[right_len] = '\0';

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

    if (var_count != 1)
    {
        free(left);
        free(right);
        return false;
    }

    size_t func_len = strlen(left) + strlen(right) + strlen("(%s)-(%s)") + 1;
    char *func = malloc(func_len);
    if (!func)
    {
        free(left);
        free(right);
        return false;
    }
    int wrote = snprintf(func, func_len, "(%s)-(%s)", left, right);
    if (wrote < 0 || (size_t)wrote >= func_len)
    {
        free(func);
        free(left);
        free(right);
        return false;
    }

    double *roots = NULL;
    int nroots = 0;
    find_roots(func, var, &roots, &nroots);
    free(func);
    free(left);
    free(right);

    if (nroots == 0)
    {
        free(roots);
        return false;
    }

    size_t out_capacity = 64;
    *output_ptr = malloc(out_capacity);
    if (!*output_ptr)
    {
        free(roots);
        return false;
    }
    size_t off = 0;
    int n = snprintf(*output_ptr + off, out_capacity - off, "%c=", var);
    if (n < 0)
        n = 0;
    off += (size_t)n;

    for (int i = 0; i < nroots; i++)
    {
        char tmp[32];
        n = snprintf(tmp, sizeof(tmp), "%g", roots[i]);
        if (n < 0)
            n = 0;

        if (off + (size_t)n + 2 >= out_capacity)
        {
            out_capacity = (off + n + 2) * 2;
            char *tmp_ptr = realloc(*output_ptr, out_capacity);
            if (!tmp_ptr)
            {
                free(roots);
                free(*output_ptr);
                *output_ptr = NULL;
                return false;
            }
            *output_ptr = tmp_ptr;
        }

        if (i > 0)
        {
            (*output_ptr)[off++] = ',';
            (*output_ptr)[off++] = ' ';
        }
        memcpy(*output_ptr + off, tmp, (size_t)n);
        off += (size_t)n;
        (*output_ptr)[off] = '\0';
    }

    free(roots);
    return true;
}

int main(int argc, char **argv)
{
    var_value = calloc(256, sizeof(double));
    if (!var_value)
        return 1;

    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            char *out = NULL;
            bool has_letter = strpbrk(argv[i], "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") != NULL;
            bool has_eq = strchr(argv[i], '=') != NULL;

            if (has_letter && has_eq)
            {
                if (!solve_equation(argv[i], &out))
                    out = strdup("Syntax error");
            }
            else if (!has_letter)
            {
                CalcResult res = calc_evaluate(argv[i]);
                if (res.status == CALC_OK)
                {
                    size_t len = strlen(argv[i]) + 32;
                    out = malloc(len);
                    snprintf(out, len, "%s = %g", argv[i], res.value);
                }
                else if (res.status == CALC_ERR_DIV0)
                {
                    size_t len = strlen(argv[i]) + 32;
                    out = malloc(len);
                    snprintf(out, len, "%s : Division by zero", argv[i]);
                }
                else
                {
                    size_t len = strlen(argv[i]) + 32;
                    out = malloc(len);
                    snprintf(out, len, "%s : Syntax error", argv[i]);
                }
            }
            else
            {
                out = strdup("Syntax error");
            }

            printf("%s\n", out);
            free(out);
        }
        free(var_value);
        return 0;
    }

    printf("\033[2J\033[1;1H");
    printf("\033[30;47m BaOS CALC utility : Type 'quit' to exit. \033[0K\033[0m\n\n");
    printf("\033[3;9999r");
    fflush(stdout);

    char *last_result = NULL;

    while (1)
    {
        printf("\033[2;1H\033[J");
        printf("\033[5;1H");

        if (last_result)
        {
            printf("%s\n\n", last_result);
            free(last_result);
            last_result = NULL;
        }

        printf("\033[3;1H\033[2K");
        printf("> ");
        fflush(stdout);

        char *buf = NULL;
        read_line(&buf);
        if (!buf)
            break;

        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n')
            buf[len - 1] = '\0';

        if (strcmp(buf, "quit") == 0)
        {
            free(buf);
            printf("\033[2J");
            break;
        }

        if (buf[0] == '\0')
        {
            free(buf);
            continue;
        }

        for (int i = 0; i < 256; i++)
            var_value[i] = 0.0;

        bool has_letter = strpbrk(buf, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") != NULL;
        bool has_eq = strchr(buf, '=') != NULL;

        if (has_letter && has_eq)
        {
            if (!solve_equation(buf, &last_result))
                last_result = strdup("Syntax error");
        }
        else if (!has_letter)
        {
            CalcResult res = calc_evaluate(buf);
            size_t out_len = strlen(buf) + 64;
            last_result = malloc(out_len);
            if (res.status == CALC_OK)
                snprintf(last_result, out_len, "%s = %g", buf, res.value);
            else if (res.status == CALC_ERR_DIV0)
                snprintf(last_result, out_len, "%s : Division by zero", buf);
            else
                snprintf(last_result, out_len, "%s : Syntax error", buf);
        }
        else
        {
            last_result = strdup("Syntax error");
        }

        free(buf);

        printf("%s\n", last_result);
        fflush(stdout);
    }

    printf("\033[r");
    fflush(stdout);

    free(last_result);
    free(var_value);
    return 0;
}
