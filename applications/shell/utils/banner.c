#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HEIGHT 11

const char *font[52][MAX_HEIGHT] = {
    {"  /$$$$$$  ",
     " /$$__  $$ ",
     "| $$  \\ $$",
     "| $$$$$$$$ ",
     "| $$__  $$ ",
     "| $$  | $$ ",
     "| $$  | $$ ",
     "|__/  |__/ ",
     "           ",
     "           ",
     "           "},

    {" /$$$$$$$  ",
     "| $$__  $$ ",
     "| $$  \\ $$",
     "| $$$$$$$  ",
     "| $$__  $$ ",
     "| $$  \\ $$",
     "| $$$$$$$/ ",
     "|_______/  ",
     "           ",
     "           ",
     "           "},

    {"  /$$$$$$  ",
     " /$$__  $$ ",
     "| $$  \\__/",
     "| $$       ",
     "| $$       ",
     "| $$    $$ ",
     "|  $$$$$$/ ",
     " \\______/ ",
     "           ",
     "           ",
     "           "},

    {" /$$$$$$$  ",
     "| $$__  $$ ",
     "| $$  \\ $$",
     "| $$  | $$ ",
     "| $$  | $$ ",
     "| $$  | $$ ",
     "| $$$$$$$/ ",
     "|_______/  ",
     "           ",
     "           ",
     "           "},

    {" /$$$$$$$$",
     "| $$_____/",
     "| $$      ",
     "| $$$$$   ",
     "| $$__/   ",
     "| $$      ",
     "| $$$$$$$$",
     "|________/",
     "          ",
     "          ",
     "          "},

    {" /$$$$$$$$",
     "| $$_____/",
     "| $$      ",
     "| $$$$$   ",
     "| $$__/   ",
     "| $$      ",
     "| $$      ",
     "|__/      ",
     "          ",
     "          ",
     "          "},

    {"  /$$$$$$  ",
     " /$$__  $$ ",
     "| $$  \\__/",
     "| $$ /$$$$ ",
     "| $$|_  $$ ",
     "| $$  \\ $$",
     "|  $$$$$$/ ",
     " \\______/ ",
     "           ",
     "           ",
     "           "},

    {" /$$   /$$",
     "| $$  | $$",
     "| $$  | $$",
     "| $$$$$$$$",
     "| $$__  $$",
     "| $$  | $$",
     "| $$  | $$",
     "|__/  |__/",
     "          ",
     "          ",
     "          "},

    {" /$$$$$$",
     "|_  $$_/",
     "  | $$  ",
     "  | $$  ",
     "  | $$  ",
     "  | $$  ",
     " /$$$$$$",
     "|______/",
     "        ",
     "        ",
     "        "},

    {"    /$$$$$",
     "   |__  $$",
     "      | $$",
     "      | $$",
     " /$$  | $$",
     "| $$  | $$",
     "|  $$$$$$/",
     " \\______/",
     "          ",
     "          ",
     "          "},

    {" /$$   /$$ ",
     "| $$  /$$/ ",
     "| $$ /$$/  ",
     "| $$$$$/   ",
     "| $$  $$   ",
     "| $$\\  $$ ",
     "| $$ \\  $$",
     "|__/  \\__/",
     "           ",
     "           ",
     "           "},

    {" /$$      ",
     "| $$      ",
     "| $$      ",
     "| $$      ",
     "| $$      ",
     "| $$      ",
     "| $$$$$$$$",
     "|________/",
     "          ",
     "          ",
     "          "},

    {" /$$      /$$ ",
     "| $$$    /$$$ ",
     "| $$$$  /$$$$ ",
     "| $$ $$/$$ $$ ",
     "| $$  $$$| $$ ",
     "| $$\\  $ | $$",
     "| $$ \\/  | $$",
     "|__/     |__/ ",
     "              ",
     "              ",
     "              "},

    {" /$$   /$$ ",
     "| $$$ | $$ ",
     "| $$$$| $$ ",
     "| $$ $$ $$ ",
     "| $$  $$$$ ",
     "| $$\\  $$$",
     "| $$ \\  $$",
     "|__/  \\__/",
     "           ",
     "           ",
     "           "},

    {"  /$$$$$$  ",
     " /$$__  $$ ",
     "| $$  \\ $$",
     "| $$  | $$ ",
     "| $$  | $$ ",
     "| $$  | $$ ",
     "|  $$$$$$/ ",
     " \\______/ ",
     "           ",
     "           ",
     "           "},

    {" /$$$$$$$  ",
     "| $$__  $$ ",
     "| $$  \\ $$",
     "| $$$$$$$/ ",
     "| $$____/  ",
     "| $$       ",
     "| $$       ",
     "|__/       ",
     "           ",
     "           ",
     "           "},

    {"  /$$$$$$  ",
     " /$$__  $$ ",
     "| $$  \\ $$",
     "| $$  | $$ ",
     "| $$  | $$ ",
     "| $$/$$ $$ ",
     "|  $$$$$$/ ",
     " \\____ $$$",
     "      \\__/",
     "           ",
     "           "},

    {" /$$$$$$$  ",
     "| $$__  $$ ",
     "| $$  \\ $$",
     "| $$$$$$$/ ",
     "| $$__  $$ ",
     "| $$  \\ $$",
     "| $$  | $$ ",
     "|__/  |__/ ",
     "           ",
     "           ",
     "           "},

    {"  /$$$$$$  ",
     " /$$__  $$ ",
     "| $$  \\__/",
     "|  $$$$$$  ",
     " \\____  $$",
     " /$$  \\ $$",
     "|  $$$$$$/ ",
     " \\______/ ",
     "           ",
     "           ",
     "           "},

    {" /$$$$$$$$",
     "|__  $$__/",
     "   | $$   ",
     "   | $$   ",
     "   | $$   ",
     "   | $$   ",
     "   | $$   ",
     "   |__/   ",
     "          ",
     "          ",
     "          "},

    {" /$$   /$$",
     "| $$  | $$",
     "| $$  | $$",
     "| $$  | $$",
     "| $$  | $$",
     "| $$  | $$",
     "|  $$$$$$/",
     " \\______/",
     "          ",
     "          ",
     "          "},

    {" /$$    /$$",
     "| $$   | $$",
     "| $$   | $$",
     "|  $$ / $$/",
     " \\  $$ $$/",
     "  \\  $$$/ ",
     "   \\  $/  ",
     "    \\_/   ",
     "           ",
     "           ",
     "           "},

    {" /$$      /$$ ",
     "| $$  /$ | $$ ",
     "| $$ /$$$| $$ ",
     "| $$/$$ $$ $$ ",
     "| $$$$_  $$$$ ",
     "| $$$/ \\  $$$",
     "| $$/   \\  $$",
     "|__/     \\__/",
     "              ",
     "              ",
     "              "},

    {" /$$   /$$ ",
     "| $$  / $$ ",
     "|  $$/ $$/ ",
     " \\  $$$$/ ",
     "  >$$  $$  ",
     " /$$/\\  $$",
     "| $$  \\ $$",
     "|__/  |__/ ",
     "           ",
     "           ",
     "           "},

    {" /$$     /$$",
     "|  $$   /$$/",
     " \\  $$ /$$/",
     "  \\  $$$$/ ",
     "   \\  $$/  ",
     "    | $$    ",
     "    | $$    ",
     "    |__/    ",
     "            ",
     "            ",
     "            "},

    {" /$$$$$$$$",
     "|_____ $$ ",
     "     /$$/ ",
     "    /$$/  ",
     "   /$$/   ",
     "  /$$/    ",
     " /$$$$$$$$",
     "|________/",
     "          ",
     "          ",
     "          "},

    {"           ",
     "           ",
     "  /$$$$$$  ",
     " |____  $$ ",
     "  /$$$$$$$ ",
     " /$$__  $$ ",
     "|  $$$$$$$ ",
     " \\_______/",
     "           ",
     "           ",
     "           "},

    {" /$$       ",
     "| $$       ",
     "| $$$$$$$  ",
     "| $$__  $$ ",
     "| $$  \\ $$",
     "| $$  | $$ ",
     "| $$$$$$$/ ",
     "|_______/  ",
     "           ",
     "           ",
     "           "},

    {"           ",
     "           ",
     "  /$$$$$$$ ",
     " /$$_____/ ",
     "| $$       ",
     "| $$       ",
     "|  $$$$$$$ ",
     " \\_______/",
     "           ",
     "           ",
     "           "},

    {"       /$$ ",
     "      | $$ ",
     "  /$$$$$$$ ",
     " /$$__  $$ ",
     "| $$  | $$ ",
     "| $$  | $$ ",
     "|  $$$$$$$ ",
     " \\_______/",
     "           ",
     "           ",
     "           "},

    {"           ",
     "           ",
     "   /$$$$$$ ",
     " /$$__  $$ ",
     "| $$$$$$$$ ",
     "| $$_____/ ",
     "|  $$$$$$$ ",
     " \\_______/",
     "           ",
     "           ",
     "           "},

    {"  /$$$$$$  ",
     " /$$__  $$ ",
     "| $$  \\__/",
     "| $$$$     ",
     "| $$_/     ",
     "| $$       ",
     "| $$       ",
     "|__/       ",
     "           ",
     "           ",
     "           "},

    {
        "           ",
        "           ",
        "  /$$$$$$  ",
        " /$$__  $$ ",
        "| $$  \\ $$",
        "| $$  | $$ ",
        "|  $$$$$$$ ",
        " \\____  $$",
        " /$$  \\ $$",
        "|  $$$$$$/ ",
        " \\______/ ",
    },

    {" /$$       ",
     "| $$       ",
     "| $$$$$$$  ",
     "| $$__  $$ ",
     "| $$  \\ $$",
     "| $$  | $$ ",
     "| $$  | $$ ",
     "|__/  |__/ ",
     "           ",
     "           ",
     "           "},

    {" /$$",
     "|__/",
     " /$$",
     "| $$",
     "| $$",
     "| $$",
     "| $$",
     "|__/",
     "    ",
     "    ",
     "    "},

    {"          ",
     "          ",
     "       /$$",
     "      |__/",
     "       /$$",
     "      | $$",
     "      | $$",
     "      | $$",
     " /$$  | $$",
     "|  $$$$$$/",
     " \\______/"},

    {" /$$       ",
     "| $$       ",
     "| $$   /$$ ",
     "| $$  /$$/ ",
     "| $$$$$$/  ",
     "| $$_  $$  ",
     "| $$ \\  $$",
     "|__/  \\__/",
     "           ",
     "           ",
     "           "},

    {" /$$",
     "| $$",
     "| $$",
     "| $$",
     "| $$",
     "| $$",
     "| $$",
     "|__/",
     "    ",
     "    ",
     "    "},

    {"                ",
     "                ",
     " /$$$$$$/$$$$   ",
     "| $$_  $$_  $$  ",
     "| $$ \\ $$ \\ $$",
     "| $$ | $$ | $$  ",
     "| $$ | $$ | $$  ",
     "|__/ |__/ |__/  ",
     "                ",
     "                ",
     "                "},

    {"           ",
     "           ",
     " /$$$$$$$  ",
     "| $$__  $$ ",
     "| $$  \\ $$",
     "| $$  | $$ ",
     "| $$  | $$ ",
     "|__/  |__/ ",
     "           ",
     "           ",
     "           "},

    {"           ",
     "           ",
     "  /$$$$$$  ",
     " /$$__  $$ ",
     "| $$  \\ $$",
     "| $$  | $$ ",
     "|  $$$$$$/ ",
     " \\______/ ",
     "           ",
     "           ",
     "           "},

    {"           ",
     "           ",
     "  /$$$$$$  ",
     " /$$__  $$ ",
     "| $$  \\ $$",
     "| $$  | $$ ",
     "| $$$$$$$/ ",
     "| $$____/  ",
     "| $$       ",
     "| $$       ",
     "|__/       "},

    {"           ",
     "           ",
     "  /$$$$$$  ",
     " /$$__  $$ ",
     "| $$  \\ $$",
     "| $$  | $$ ",
     "|  $$$$$$$ ",
     " \\____  $$",
     "      | $$ ",
     "      | $$ ",
     "      |__/ "},

    {"           ",
     "           ",
     "  /$$$$$$  ",
     " /$$__  $$ ",
     "| $$  \\__/",
     "| $$       ",
     "| $$       ",
     "|__/       ",
     "           ",
     "           ",
     "           "},

    {"           ",
     "           ",
     "  /$$$$$$$ ",
     " /$$_____/ ",
     "|  $$$$$$  ",
     " \\____  $$",
     " /$$$$$$$/ ",
     "|_______/  ",
     "           ",
     "           ",
     "           "},

    {"   /$$    ",
     "  | $$    ",
     " /$$$$$$  ",
     "|_  $$_/  ",
     "  | $$    ",
     "  | $$ /$$",
     "  |  $$$$/",
     "   \\___/ ",
     "          ",
     "          ",
     "          "},

    {"          ",
     "          ",
     " /$$   /$$",
     "| $$  | $$",
     "| $$  | $$",
     "| $$  | $$",
     "|  $$$$$$/",
     " \\______/",
     "          ",
     "          ",
     "          "},

    {"           ",
     "           ",
     " /$$    /$$",
     "|  $$  /$$/",
     " \\  $$/$$/",
     "  \\  $$$/ ",
     "   \\  $/  ",
     "    \\_/   ",
     "           ",
     "           ",
     "           "},

    {"               ",
     "               ",
     " /$$  /$$  /$$ ",
     "| $$ | $$ | $$ ",
     "| $$ | $$ | $$ ",
     "| $$ | $$ | $$ ",
     "|  $$$$$/$$$$/ ",
     " \\_____/\\___/",
     "               ",
     "               ",
     "               "},

    {"           ",
     "           ",
     " /$$   /$$ ",
     "|  $$ /$$/ ",
     " \\  $$$$/ ",
     "  >$$  $$  ",
     " /$$/\\  $$",
     "|__/  \\__/",
     "           ",
     "           ",
     "           "},

    {"           ",
     "           ",
     " /$$   /$$ ",
     "| $$  | $$ ",
     "| $$  | $$ ",
     "| $$  | $$ ",
     "|  $$$$$$$ ",
     " \\____  $$",
     " /$$  | $$ ",
     "|  $$$$$$/ ",
     " \\______/ "},

    {"          ",
     "          ",
     " /$$$$$$$$",
     "|____ /$$/",
     "   /$$$$/ ",
     "  /$$__/  ",
     " /$$$$$$$$",
     "|________/",
     "          ",
     "          ",
     "          "},
};

const char *digits[10][MAX_HEIGHT] = {
    {"  /$$$$$$  ",
     " /$$$_  $$ ",
     "| $$$$\\ $$",
     "| $$ $$ $$ ",
     "| $$\\ $$$$",
     "| $$ \\ $$$",
     "  $$$$$$/  ",
     "  \\______/",
     "           ",
     "           ",
     "           "},

    {"   /$$   ",
     " /$$$$   ",
     "|_  $$   ",
     "  | $$   ",
     "  | $$   ",
     "  | $$   ",
     " /$$$$$$ ",
     "|______ /",
     "         ",
     "         ",
     "         "},

    {"  /$$$$$$  ",
     " /$$__  $$ ",
     "|__/  \\ $$",
     "   /$$$$$/ ",
     "  /$$____/ ",
     " | $$      ",
     " | $$$$$$$$",
     " |________/",
     "           ",
     "           ",
     "           "},

    {"  /$$$$$$  ",
     " /$$__  $$ ",
     "|__/  \\ $$",
     "   /$$$$$/ ",
     "  |___  $$ ",
     " /$$  \\ $$",
     "|  $$$$$$/ ",
     " \\______/ ",
     "           ",
     "           ",
     "           "},

    {" /$$   /$$",
     "| $$  | $$",
     "| $$  | $$",
     "| $$$$$$$$",
     "|_____  $$",
     "      | $$",
     "      | $$",
     "      |__/",
     "          ",
     "          ",
     "          "},

    {" /$$$$$$$  ",
     "| $$____/  ",
     "| $$       ",
     "| $$$$$$$  ",
     "|_____  $$ ",
     " /$$  \\ $$",
     "|  $$$$$$/ ",
     " \\______/ ",
     "           ",
     "           ",
     "           "},

    {"  /$$$$$$  ",
     " /$$__  $$ ",
     "| $$  \\__/",
     "| $$$$$$$  ",
     "| $$__  $$ ",
     "| $$  \\ $$",
     "|  $$$$$$/ ",
     " \\______/ ",
     "           ",
     "           ",
     "           "},

    {" /$$$$$$$$ ",
     "|_____ $$/ ",
     "     /$$/  ",
     "    /$$/   ",
     "   /$$/    ",
     "  /$$/     ",
     " /$$/      ",
     "|__/       ",
     "           ",
     "           ",
     "           "},

    {"  /$$$$$$  ",
     " /$$__  $$ ",
     "| $$  \\ $$",
     "|  $$$$$$/ ",
     " >$$__  $$ ",
     "| $$  \\ $$",
     "|  $$$$$$/ ",
     " \\______/ ",
     "           ",
     "           ",
     "           "},

    {"  /$$$$$$  ",
     " /$$__  $$ ",
     "| $$  \\ $$",
     "|  $$$$$$$ ",
     " \\____  $$ ",
     " /$$  \\ $$ ",
     "|  $$$$$$/ ",
     " \\______/ ",
     "           ",
     "           ",
     "           "}};

static int glyph_width_cache[256];
static int glyph_cache_inited = 0;

const char **get_font(char c)
{
    if (c >= 'A' && c <= 'Z')
        return font[c - 'A'];
    else if (c >= 'a' && c <= 'z')
        return font[c - 'a' + 26];
    else if (c >= '0' && c <= '9')
        return digits[c - '0'];
    else if (c == ' ')
    {
        static const char *empty[MAX_HEIGHT] = {"     ", "     ", "     ", "     ", "     ", "     ", "     ", "     "};
        return empty;
    }
    else
        return NULL;
}

int glyph_width_for_char(char ch)
{
    if (!glyph_cache_inited)
    {
        for (int i = 0; i < 256; ++i)
            glyph_width_cache[i] = -1;
        glyph_cache_inited = 1;
    }

    unsigned char u = (unsigned char)ch;
    if (glyph_width_cache[u] != -1)
        return glyph_width_cache[u];

    const char **g = get_font(ch);
    if (!g)
    {
        glyph_width_cache[u] = 0;
        return 0;
    }

    int maxlen = 0;
    for (int r = 0; r < MAX_HEIGHT; ++r)
    {
        if (!g[r])
            continue;
        int l = (int)strlen(g[r]);
        if (l > maxlen)
            maxlen = l;
    }

    if (maxlen == 0)
        maxlen = 1;

    glyph_width_cache[u] = maxlen;
    return maxlen;
}

int main(int argc, char *argv[])
{
    if (argc <= 1)
    {
        printf("\033[1;33mUsage:\033[0m banner [-color=<color>] <text>.\n");
        return 1;
    }

    const int SCREEN_W = 80;
    const int SCREEN_H = 25;
    const int LINE_HEIGHT = MAX_HEIGHT + 1;
    const int MAX_TEXT_LINES = SCREEN_H / LINE_HEIGHT;

    int glyph_spacing = 1;
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

    const char *color_prefix = NULL;
    const char *color_reset = "\033[0m";

    int first_word_seen = 0;

    for (int i = 1; i < argc; ++i)
    {

        if (strncmp(argv[i], "-color=", 7) == 0)
        {
            const char *val = argv[i] + 7;

            if (strcmp(val, "gray") == 0 || strcmp(val, "grey") == 0)
                color_prefix = "\033[1;30m";
            else if (strcmp(val, "red") == 0)
                color_prefix = "\033[0;31m";
            else if (strcmp(val, "green") == 0)
                color_prefix = "\033[0;32m";
            else if (strcmp(val, "yellow") == 0)
                color_prefix = "\033[0;33m";
            else if (strcmp(val, "blue") == 0)
                color_prefix = "\033[0;34m";
            else if (strcmp(val, "magenta") == 0)
                color_prefix = "\033[0;35m";
            else if (strcmp(val, "cyan") == 0)
                color_prefix = "\033[0;36m";

            else if (strcmp(val, "lightgray") == 0 || strcmp(val, "lightgrey") == 0)
                color_prefix = "\033[0;37m";
            else if (strcmp(val, "lightred") == 0)
                color_prefix = "\033[1;31m";
            else if (strcmp(val, "lightgreen") == 0)
                color_prefix = "\033[1;32m";
            else if (strcmp(val, "lightyellow") == 0)
                color_prefix = "\033[1;33m";
            else if (strcmp(val, "lightblue") == 0)
                color_prefix = "\033[1;34m";
            else if (strcmp(val, "lightmagenta") == 0)
                color_prefix = "\033[1;35m";
            else if (strcmp(val, "lightcyan") == 0)
                color_prefix = "\033[1;36m";

            else if (strcmp(val, "white") == 0)
                color_prefix = "\033[1;37m";

            else
                color_prefix = NULL;

            continue;
        }

        if (first_word_seen)
        {
            int sep_w = glyph_width_for_char(' ') + glyph_spacing;
            if (current_width + sep_w > SCREEN_W)
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
            if (current_width + sep_w <= SCREEN_W)
            {
                size_t idx = line_len[lines_count - 1];
                if (idx + 1 >= line_cap[lines_count - 1])
                {
                    line_cap[lines_count - 1] *= 2;
                    lines[lines_count - 1] = realloc(lines[lines_count - 1], line_cap[lines_count - 1]);
                }
                lines[lines_count - 1][idx] = ' ';
                line_len[lines_count - 1]++;
                current_width += sep_w;
            }
        }

        size_t len = strlen(argv[i]);
        for (size_t j = 0; j < len; ++j)
        {
            char ch = argv[i][j];

            int char_w = glyph_width_for_char(ch) + glyph_spacing;
            if (current_width + char_w > SCREEN_W)
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

            if ((int)lines_count > 0)
            {
                size_t idx = line_len[lines_count - 1];
                if (idx + 1 >= line_cap[lines_count - 1])
                {
                    line_cap[lines_count - 1] *= 2;
                    lines[lines_count - 1] = realloc(lines[lines_count - 1], line_cap[lines_count - 1]);
                }
                lines[lines_count - 1][idx] = ch;
                line_len[lines_count - 1]++;
                current_width += char_w;
            }
        }

        if (truncated)
            break;

        first_word_seen = 1;
    }

    for (size_t L = 0; L < lines_count; ++L)
    {
        int any_vertical = 0;
        for (size_t k = 0; k < line_len[L]; ++k)
        {
            char ch = lines[L][k];
            const char **g = get_font(ch);
            if (!g)
                continue;
            for (int r = 0; r < MAX_HEIGHT; ++r)
            {
                if (g[r] && g[r][0] != '\0')
                {
                    any_vertical = 1;
                    break;
                }
            }
            if (any_vertical)
                break;
        }

        if (!any_vertical)
            continue;

        for (int row = 0; row < MAX_HEIGHT; ++row)
        {
            if (color_prefix)
                printf("%s", color_prefix);

            for (size_t k = 0; k < line_len[L]; ++k)
            {
                char ch = lines[L][k];
                if (ch == ' ')
                {
                    int w = glyph_width_for_char(' ');
                    for (int i = 0; i < w; ++i)
                        putchar(' ');
                    for (int sp = 0; sp < glyph_spacing; ++sp)
                        putchar(' ');
                    continue;
                }

                const char **g = get_font(ch);
                if (!g)
                    continue;

                int w = glyph_width_for_char(ch);

                const char *s = g[row];
                int len = 0;
                if (s && s[0] != '\0')
                {
                    printf("%s", s);
                    len = (int)strlen(s);
                }

                for (int sidx = len; sidx < w; ++sidx)
                    putchar(' ');

                for (int sp = 0; sp < glyph_spacing; ++sp)
                    putchar(' ');
            }

            if (color_prefix)
                printf("%s", color_reset);

            putchar('\n');
        }

        putchar('\n');
    }

    if (truncated)
        printf("\033[1;33mWarning: Output truncated to fit %dx%d (max %d text rows).\033[0m\n",
               SCREEN_W, SCREEN_H, MAX_TEXT_LINES);

    for (size_t i = 0; i < lines_count; ++i)
        free(lines[i]);
    free(lines);
    free(line_len);
    free(line_cap);

    return 0;
}