/* test11.c - cast, sizeof, hex
* Expected: baoc OK, exit code 13
* Tests: cast, sizeof, hex
*/
struct Point
{
    int x;
    int y;
};

int main()
{
    struct Point p;
    int a = (int)0x0A;
    int sz = (int)sizeof(struct Point);
    p.x = a;
    p.y = 1;
    return p.x + p.y + sz;
}
