/* test4.c - types, preprocessor, struct, enum, typedef
 * Expected: baoc OK, exit code 10  (MAGIC 7 + RED 1 + GREEN 2)
 * Tests: #define, typedef, struct, enum, signed/unsigned, char
 */
#define MAGIC 7

typedef struct
{
    int x;
    int y;
} Point;

enum Color
{
    RED = 1,
    GREEN = 2,
    BLUE = 3
};

int main()
{
    Point p;
    unsigned char tag;
    signed int total;

    p.x = MAGIC;
    p.y = RED;
    tag = 'A';
    total = p.x + p.y + GREEN;

    if (tag == 65)
        return total;
    return 0;
}
