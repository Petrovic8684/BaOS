#include "baoc_internal.h"

void die(const char *m)
{
    fprintf(stderr, "ERR: %s\n", m);
    fprintf(stderr, "Current token: type=%d ", curtok.type);
    if (curtok.type == TOK_IDENT)
        fprintf(stderr, "IDENT '%s'\n", curtok.ident);
    else if (curtok.type == TOK_NUMBER)
        fprintf(stderr, "NUMBER %d\n", curtok.val);
    else if (curtok.type == TOK_STRING)
        fprintf(stderr, "STRING \"%s\"\n", curtok.str);
    else if (curtok.type == TOK_SYM)
        fprintf(stderr, "SYM '%c'\n", curtok.sym);
    else if (curtok.type == TOK_OP)
    {
        const char *opname = "??";
        if (curtok.op == OP_EQ)
            opname = "==";
        else if (curtok.op == OP_NE)
            opname = "!=";
        else if (curtok.op == OP_LE)
            opname = "<=";
        else if (curtok.op == OP_GE)
            opname = ">=";
        fprintf(stderr, "OP %s\n", opname);
    }
    else
        fprintf(stderr, "EOF\n");
    if (src)
    {
        char buf[61];
        strncpy(buf, src, 60);
        buf[60] = '\0';
        fprintf(stderr, "Remaining source (around current pos): \"%s\"\n", buf);
    }

    exit(1);
}
