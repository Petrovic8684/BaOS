#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HEIGHT 8

const char *font[52][MAX_HEIGHT] = {
    {"  A  ",
     " A A ",
     "A   A",
     "A   A",
     "AAAAA",
     "A   A",
     "A   A",
     "     "},
    {"BBBB ",
     "B   B",
     "B   B",
     "BBBB ",
     "B   B",
     "B   B",
     "BBBB ",
     "     "},
    {" CCCC",
     "C    ",
     "C    ",
     "C    ",
     "C    ",
     "C    ",
     " CCCC",
     "     "},
    {"DDDD ",
     "D   D",
     "D   D",
     "D   D",
     "D   D",
     "D   D",
     "DDDD ",
     "     "},
    {"EEEEE",
     "E    ",
     "E    ",
     "EEE  ",
     "E    ",
     "E    ",
     "EEEEE",
     "     "},
    {"FFFFF",
     "F    ",
     "F    ",
     "FFF  ",
     "F    ",
     "F    ",
     "F    ",
     "     "},
    {" GGGG",
     "G    ",
     "G    ",
     "G  GG",
     "G   G",
     "G   G",
     " GGG ",
     "     "},
    {"H   H",
     "H   H",
     "H   H",
     "HHHHH",
     "H   H",
     "H   H",
     "H   H",
     "     "},
    {"IIIII",
     "  I  ",
     "  I  ",
     "  I  ",
     "  I  ",
     "  I  ",
     "IIIII",
     "     "},
    {"JJJJJ",
     "    J",
     "    J",
     "    J",
     "J   J",
     "J   J",
     " JJJ ",
     "     "},
    {"K   K",
     "K  K ",
     "K K  ",
     "KK   ",
     "K K  ",
     "K  K ",
     "K   K",
     "     "},
    {"L    ",
     "L    ",
     "L    ",
     "L    ",
     "L    ",
     "L    ",
     "LLLLL",
     "     "},
    {"M   M",
     "MM MM",
     "M M M",
     "M   M",
     "M   M",
     "M   M",
     "M   M",
     "     "},
    {"N   N",
     "NN  N",
     "N N N",
     "N  NN",
     "N   N",
     "N   N",
     "N   N",
     "     "},
    {" OOO ",
     "O   O",
     "O   O",
     "O   O",
     "O   O",
     "O   O",
     " OOO ",
     "     "},
    {"PPPP ",
     "P   P",
     "P   P",
     "PPPP ",
     "P    ",
     "P    ",
     "P    ",
     "     "},
    {" QQQ ",
     "Q   Q",
     "Q   Q",
     "Q   Q",
     "Q Q Q",
     "Q  Q ",
     " QQ Q",
     "     "},
    {"RRRR ",
     "R   R",
     "R   R",
     "RRRR ",
     "R R  ",
     "R  R ",
     "R   R",
     "     "},
    {" SSSS",
     "S    ",
     "S    ",
     " SSS ",
     "    S",
     "    S",
     "SSSS ",
     "     "},
    {"TTTTT",
     "  T  ",
     "  T  ",
     "  T  ",
     "  T  ",
     "  T  ",
     "  T  ",
     "     "},
    {"U   U",
     "U   U",
     "U   U",
     "U   U",
     "U   U",
     "U   U",
     " UUU ",
     "     "},
    {"V   V",
     "V   V",
     "V   V",
     "V   V",
     " V V ",
     " V V ",
     "  V  ",
     "     "},
    {"W   W",
     "W   W",
     "W   W",
     "W   W",
     "W W W",
     "WW WW",
     "W   W",
     "     "},
    {"X   X",
     " X X ",
     " X X ",
     "  X  ",
     " X X ",
     " X X ",
     "X   X",
     "     "},
    {"Y   Y",
     " Y Y ",
     " Y Y ",
     "  Y  ",
     "  Y  ",
     "  Y  ",
     "  Y  ",
     "     "},
    {"ZZZZZ",
     "   Z ",
     "  Z  ",
     " Z   ",
     "Z    ",
     "Z    ",
     "ZZZZZ",
     "     "},

    {"     ",
     " aaa ",
     "    a",
     " aaaa",
     "a   a",
     "a   a",
     " aaaa",
     "     "},
    {"b    ",
     "b    ",
     "b bb ",
     "bb  b",
     "b   b",
     "b   b",
     "bb b ",
     "     "},
    {"     ",
     "     ",
     " ccc ",
     "c    ",
     "c    ",
     "c    ",
     " ccc ",
     "     "},
    {"    d",
     "    d",
     " dddd",
     "d   d",
     "d   d",
     "d   d",
     " ddd ",
     "     "},
    {"     ",
     "     ",
     " eee ",
     "e   e",
     "eeeee",
     "e    ",
     " eee ",
     "     "},
    {"  fff",
     " f   ",
     " fff ",
     " f   ",
     " f   ",
     " f   ",
     " f   ",
     "     "},
    {"     ",
     " ggg ",
     "g   g",
     " ggg ",
     "    g",
     "g   g",
     " ggg ",
     "     "},
    {"h    ",
     "h    ",
     "h hh ",
     "hh  h",
     "h   h",
     "h   h",
     "h   h",
     "     "},
    {"  i  ",
     "     ",
     " ii  ",
     "  i  ",
     "  i  ",
     "  i  ",
     " iii ",
     "     "},
    {"   j ",
     "     ",
     "   j ",
     "   j ",
     "j  j ",
     "j  j ",
     " jj  ",
     "     "},
    {"k    ",
     "k    ",
     "k k  ",
     "kk   ",
     "k k  ",
     "k  k ",
     "k   k",
     "     "},
    {" ll  ",
     "  l  ",
     "  l  ",
     "  l  ",
     "  l  ",
     "  l  ",
     " lll ",
     "     "},
    {"     ",
     "m m m",
     "mm mm",
     "m m m",
     "m   m",
     "m   m",
     "m   m",
     "     "},
    {"     ",
     "n n  ",
     "nn n ",
     "n nn ",
     "n  n ",
     "n  n ",
     "n  n ",
     "     "},
    {"     ",
     " ooo ",
     "o   o",
     "o   o",
     "o   o",
     "o   o",
     " ooo ",
     "     "},
    {"     ",
     "ppp  ",
     "p  p ",
     "ppp  ",
     "p    ",
     "p    ",
     "p    ",
     "     "},
    {"     ",
     " qqq ",
     "q   q",
     "q   q",
     " qqq ",
     "    q",
     "    q",
     "     "},
    {"     ",
     "r rr ",
     "rr  r",
     "r    ",
     "r    ",
     "r    ",
     "r    ",
     "     "},
    {"     ",
     " sss ",
     "s    ",
     " sss ",
     "    s",
     "s   s",
     " sss ",
     "     "},
    {" t   ",
     " t   ",
     " ttt ",
     " t   ",
     " t   ",
     " t   ",
     "  tt ",
     "     "},
    {"     ",
     "u   u",
     "u   u",
     "u   u",
     "u   u",
     "u  uu",
     " uuu ",
     "     "},
    {"     ",
     "v   v",
     "v   v",
     "v   v",
     " v v ",
     " v v ",
     "  v  ",
     "     "},
    {"     ",
     "w   w",
     "w   w",
     "w w w",
     "ww ww",
     "ww ww",
     "w   w",
     "     "},
    {"     ",
     "x   x",
     " x x ",
     "  x  ",
     " x x ",
     "x   x",
     "     ",
     "     "},
    {"     ",
     "y   y",
     " y y ",
     "  y  ",
     "  y  ",
     "  y  ",
     " yy  ",
     "     "},
    {"     ",
     "zzzzz",
     "   z ",
     "  z  ",
     " z   ",
     "z    ",
     "zzzzz",
     "     "}};

const char *digits[10][MAX_HEIGHT] = {
    {" 000 ",
     "0   0",
     "0  00",
     "0 0 0",
     "00  0",
     "0   0",
     " 000 ",
     "     "},
    {"  1  ",
     " 11  ",
     "  1  ",
     "  1  ",
     "  1  ",
     "  1  ",
     " 111 ",
     "     "},
    {" 222 ",
     "2   2",
     "    2",
     "   2 ",
     "  2  ",
     " 2   ",
     "22222",
     "     "},
    {" 333 ",
     "3   3",
     "    3",
     "  33 ",
     "    3",
     "3   3",
     " 333 ",
     "     "},
    {"   4 ",
     "  44 ",
     " 4 4 ",
     "4  4 ",
     "44444",
     "   4 ",
     "   4 ",
     "     "},
    {"55555",
     "5    ",
     "5    ",
     "5555 ",
     "    5",
     "5   5",
     " 555 ",
     "     "},
    {" 666 ",
     "6    ",
     "6    ",
     "6666 ",
     "6   6",
     "6   6",
     " 666 ",
     "     "},
    {"77777",
     "    7",
     "   7 ",
     "  7  ",
     " 7   ",
     "7    ",
     "7    ",
     "     "},
    {" 888 ",
     "8   8",
     "8   8",
     " 888 ",
     "8   8",
     "8   8",
     " 888 ",
     "     "},
    {" 999 ",
     "9   9",
     "9   9",
     " 9999",
     "    9",
     "    9",
     " 999 ",
     "     "}};

const char *symbols[][MAX_HEIGHT] = {
    {"  !  ",
     "  !  ",
     "  !  ",
     "  !  ",
     "  !  ",
     "     ",
     "  !  ",
     "     "},
    {" @@@ ",
     "@   @",
     "@ @ @",
     "@ @ @",
     "@  @@",
     "@    ",
     " @@@ ",
     "     "},
    {"     ",
     "     ",
     " # # ",
     "#####",
     " # # ",
     "#####",
     " # # ",
     "     "},
    {"  $  ",
     " $$$ ",
     "$$   ",
     " $$$ ",
     "   $$",
     "$$$  ",
     "  $  ",
     "     "},
    {"     ",
     "     ",
     "%   %",
     "   % ",
     "  %  ",
     " %   ",
     "%   %",
     "     "},
    {"  ^  ",
     " ^ ^ ",
     "     ",
     "     ",
     "     ",
     "     ",
     "     ",
     "     "},
    {"  &  ",
     " & & ",
     "  &  ",
     " & & ",
     "&  & ",
     " & & ",
     "  && ",
     "     "},
    {"     ",
     "  *  ",
     " * * ",
     "*****",
     " * * ",
     "  *  ",
     "     ",
     "     "},
    {"     ",
     "   ( ",
     "  (  ",
     " (   ",
     " (   ",
     "  (  ",
     "   ( ",
     "     "},
    {"     ",
     " )   ",
     "  )  ",
     "   ) ",
     "   ) ",
     "  )  ",
     " )   ",
     "     "},
    {"     ",
     "     ",
     "     ",
     "     ",
     "     ",
     "     ",
     "_____",
     "     "},
    {"     ",
     "     ",
     "  +  ",
     "  +  ",
     "+++++",
     "  +  ",
     "  +  ",
     "     "},
    {"     ",
     "     ",
     "     ",
     "-----",
     "     ",
     "     ",
     "     ",
     "     "},
    {"     ",
     "     ",
     "    /",
     "   / ",
     "  /  ",
     " /   ",
     "/    ",
     "     "},
    {"     ",
     "  ?  ",
     " ? ? ",
     "   ? ",
     "  ?  ",
     "     ",
     "  ?  ",
     "     "},
    {"     ",
     "     ",
     "   < ",
     "  <  ",
     " <   ",
     "  <  ",
     "   < ",
     "     "},
    {"     ",
     "     ",
     " >   ",
     "  >  ",
     "   > ",
     "  >  ",
     " >   ",
     "     "},
    {"  |  ",
     "  |  ",
     "  |  ",
     "  |  ",
     "  |  ",
     "  |  ",
     "  |  ",
     "     "},
    {"     ",
     "     ",
     "\\    ",
     " \\   ",
     "  \\  ",
     "   \\ ",
     "    \\",
     "     "},
};

const char **get_font(char c)
{
    if (c >= 'A' && c <= 'Z')
        return font[c - 'A'];
    else if (c >= 'a' && c <= 'z')
        return font[c - 'a' + 26];
    else if (c >= '0' && c <= '9')
        return digits[c - '0'];
    else
    {
        switch (c)
        {
        case '!':
            return symbols[0];
        case '@':
            return symbols[1];
        case '#':
            return symbols[2];
        case '$':
            return symbols[3];
        case '%':
            return symbols[4];
        case '^':
            return symbols[5];
        case '&':
            return symbols[6];
        case '*':
            return symbols[7];
        case '(':
            return symbols[8];
        case ')':
            return symbols[9];
        case '_':
            return symbols[10];
        case '+':
            return symbols[11];
        case '-':
            return symbols[12];
        case '/':
            return symbols[13];
        case '?':
            return symbols[14];
        case '<':
            return symbols[15];
        case '>':
            return symbols[16];
        case '|':
            return symbols[17];
        case '\\':
            return symbols[18];
        default:
            static const char *empty[MAX_HEIGHT] = {"     ", "     ", "     ", "     ", "     ", "     ", "     ", "     "};
            return empty;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc <= 1)
    {
        printf("\033[31mError: Invalid text provided.\033[0m\n");
        return 1;
    }

    const int SCREEN_W = 80;
    const int SCREEN_H = 25;
    const int LINE_HEIGHT = MAX_HEIGHT + 1;
    const int MAX_TEXT_LINES = SCREEN_H / LINE_HEIGHT;

    int glyph_w = (int)strlen(font[0][0]);
    int glyph_spacing = 1;
    int glyph_total_w = glyph_w + glyph_spacing;

    size_t lines_cap = 16;
    size_t lines_count = 1;
    char **lines = malloc(sizeof(char *) * lines_cap);
    size_t *line_len = malloc(sizeof(size_t) * lines_cap);
    size_t *line_cap = malloc(sizeof(size_t) * lines_cap);

    lines[0] = malloc(128);
    line_cap[0] = 128;
    line_len[0] = 0;
    int current_width = 0;
    int truncated = 0;

    for (int i = 1; i < argc; ++i)
    {
        if (i > 1)
        {
            if (current_width + glyph_total_w > SCREEN_W)
            {
                if ((int)lines_count >= MAX_TEXT_LINES)
                {
                    truncated = 1;
                    break;
                }
                if (lines_count >= lines_cap)
                {
                    lines_cap *= 2;
                    lines = realloc(lines, sizeof(char *) * lines_cap);
                    line_len = realloc(line_len, sizeof(size_t) * lines_cap);
                    line_cap = realloc(line_cap, sizeof(size_t) * lines_cap);
                }
                lines[lines_count] = malloc(128);
                line_cap[lines_count] = 128;
                line_len[lines_count] = 0;
                lines_count++;
                current_width = 0;
            }
            size_t idx = line_len[lines_count - 1];
            if (idx + 1 >= line_cap[lines_count - 1])
            {
                line_cap[lines_count - 1] *= 2;
                lines[lines_count - 1] = realloc(lines[lines_count - 1], line_cap[lines_count - 1]);
            }
            lines[lines_count - 1][idx] = ' ';
            line_len[lines_count - 1]++;
            current_width += glyph_total_w;
        }

        size_t len = strlen(argv[i]);
        for (size_t j = 0; j < len; ++j)
        {
            char ch = argv[i][j];

            if (current_width + glyph_total_w > SCREEN_W)
            {
                if ((int)lines_count >= MAX_TEXT_LINES)
                {
                    truncated = 1;
                    break;
                }
                if (lines_count >= lines_cap)
                {
                    lines_cap *= 2;
                    lines = realloc(lines, sizeof(char *) * lines_cap);
                    line_len = realloc(line_len, sizeof(size_t) * lines_cap);
                    line_cap = realloc(line_cap, sizeof(size_t) * lines_cap);
                }
                lines[lines_count] = malloc(128);
                line_cap[lines_count] = 128;
                line_len[lines_count] = 0;
                lines_count++;
                current_width = 0;
            }

            size_t idx = line_len[lines_count - 1];
            if (idx + 1 >= line_cap[lines_count - 1])
            {
                line_cap[lines_count - 1] *= 2;
                lines[lines_count - 1] = realloc(lines[lines_count - 1], line_cap[lines_count - 1]);
            }
            lines[lines_count - 1][idx] = ch;
            line_len[lines_count - 1]++;
            current_width += glyph_total_w;
        }

        if (truncated)
            break;
    }

    if (truncated)
        printf("\033[1;33mWarning: output truncated to fit %dx%d (max %d text rows).\033[0m\n",
               SCREEN_W, SCREEN_H, MAX_TEXT_LINES);

    for (size_t L = 0; L < lines_count; ++L)
    {
        for (int row = 0; row < MAX_HEIGHT; ++row)
        {
            for (size_t k = 0; k < line_len[L]; ++k)
            {
                char ch = lines[L][k];
                const char **g = get_font(ch);
                printf("%s ", g[row]);
            }
            printf("\n");
        }
        printf("\n");
    }

    for (size_t i = 0; i < lines_count; ++i)
        free(lines[i]);
    free(lines);
    free(line_len);
    free(line_cap);

    return 0;
}