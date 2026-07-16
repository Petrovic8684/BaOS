#include "baoc_internal.h"

char *skip_ws(char *p)
{
    while (*p && isspace((unsigned char)*p))
        p++;
    return p;
}

void next_token()
{
    curtok.type = TOK_EOF;
    curtok.ident[0] = 0;
    curtok.val = 0;
    curtok.sym = 0;
    curtok.op = 0;

    while (isspace((unsigned char)*src))
        src++;

    if (!*src)
        return;

    if (isalpha((unsigned char)*src) || *src == '_')
    {
        char *p = curtok.ident;
        while (isalnum((unsigned char)*src) || *src == '_')
            *p++ = *src++;
        *p = 0;
        curtok.type = TOK_IDENT;
        return;
    }

    if (isdigit((unsigned char)*src) ||
        (*src == '-' && (isdigit((unsigned char)*(src + 1)) || src[1] == 'x' || src[1] == 'X')) ||
        (*src == '0' && (src[1] == 'x' || src[1] == 'X')))
    {
        char *end;
        int base = 10;
        if (*src == '0' && (src[1] == 'x' || src[1] == 'X'))
            base = 16;
        else if (*src == '0' && src[1] >= '0' && src[1] <= '7')
            base = 8;
        curtok.val = (int32_t)strtol(src, &end, base);
        curtok.type = TOK_NUMBER;
        src = end;
        while (*src == 'u' || *src == 'U' || *src == 'l' || *src == 'L')
            src++;
        return;
    }

    if (src[0] == '&' && src[1] == '&')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_LAND;
        src += 2;
        return;
    }
    if (src[0] == '|' && src[1] == '|')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_LOR;
        src += 2;
        return;
    }

    if ((src[0] == '=' && src[1] == '=') ||
        (src[0] == '!' && src[1] == '=') ||
        (src[0] == '<' && src[1] == '=') ||
        (src[0] == '>' && src[1] == '='))
    {
        curtok.type = TOK_OP;
        if (src[0] == '=' && src[1] == '=')
            curtok.op = OP_EQ;
        else if (src[0] == '!' && src[1] == '=')
            curtok.op = OP_NE;
        else if (src[0] == '<' && src[1] == '=')
            curtok.op = OP_LE;
        else
            curtok.op = OP_GE;
        src += 2;
        return;
    }

    if (*src == '!')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_LNOT;
        src++;
        return;
    }
    if (*src == '&')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_BAND;
        src++;
        return;
    }
    if (*src == '|')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_BOR;
        src++;
        return;
    }
    if (*src == '^')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_BXOR;
        src++;
        return;
    }
    if (*src == '~')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_COMPL;
        src++;
        return;
    }

    if (src[0] == '<' && src[1] == '<')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_SHL;
        src += 2;
        return;
    }
    if (src[0] == '>' && src[1] == '>')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_SHR;
        src += 2;
        return;
    }

    if (src[0] == '+' && src[1] == '+')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_INC;
        src += 2;
        return;
    }
    if (src[0] == '-' && src[1] == '>')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_ARROW;
        src += 2;
        return;
    }
    if (src[0] == '-' && src[1] == '-')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_DEC;
        src += 2;
        return;
    }

    if (*src == '<')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_LT;
        src++;
        return;
    }
    if (*src == '>')
    {
        curtok.type = TOK_OP;
        curtok.op = OP_GT;
        src++;
        return;
    }

    if (*src == '"')
    {
        src++;
        char *p = curtok.str;
        while (*src && *src != '"')
        {
            if (*src == '\\')
            {
                src++;
                if (!*src)
                    break;
                switch (*src)
                {
                case 'n':
                    *p++ = '\n';
                    break;
                case 't':
                    *p++ = '\t';
                    break;
                case '\\':
                    *p++ = '\\';
                    break;
                case '"':
                    *p++ = '"';
                    break;
                case '\'':
                    *p++ = '\'';
                    break;
                case '0':
                    *p++ = '\0';
                    break;
                default:
                    *p++ = *src;
                    break;
                }
                src++;
            }
            else
            {
                *p++ = *src++;
            }
        }
        *p = '\0';
        if (*src != '"')
            die("unterminated string literal");
        src++;
        curtok.type = TOK_STRING;
        return;
    }

    if (*src == '\'')
    {
        src++;
        int v = 0;
        if (*src == '\\')
        {
            src++;
            if (!*src)
                die("unterminated char literal");
            switch (*src)
            {
            case 'n':
                v = '\n';
                break;
            case 't':
                v = '\t';
                break;
            case '\\':
                v = '\\';
                break;
            case '\'':
                v = '\'';
                break;
            case '0':
                v = '\0';
                break;
            default:
                v = (unsigned char)*src;
                break;
            }
            src++;
        }
        else
        {
            if (!*src)
                die("unterminated char literal");
            v = (unsigned char)*src;
            src++;
        }
        if (*src != '\'')
            die("unterminated char literal");
        src++;
        curtok.type = TOK_NUMBER;
        curtok.val = v;
        return;
    }

    curtok.type = TOK_SYM;
    curtok.sym = *src++;
}


int accept_ident(const char *s)
{
    if (curtok.type == TOK_IDENT && strcmp(curtok.ident, s) == 0)
    {
        next_token();
        return 1;
    }
    return 0;
}


int accept_sym(char c)
{
    if (curtok.type == TOK_SYM && curtok.sym == c)
    {
        next_token();
        return 1;
    }
    return 0;
}


void expect_sym(char c)
{
    if (!accept_sym(c))
        die("expected symbol");
}


void expect_ident()
{
    if (curtok.type != TOK_IDENT)
        die("expected identifier");
}
