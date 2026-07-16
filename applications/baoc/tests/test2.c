/* test2.c - control flow and arithmetic
 * Expected: baoc OK, exit code 15  (10 + 5)
 * Tests: if/else-if/else, for, while, do-while, const, operators
 */
int main()
{
    int sum = 0;
    int i;

    for (i = 0; i < 5; i = i + 1)
        sum = sum + i;

    if (sum < 10)
        sum = 10;
    else if (sum > 20)
        sum = 20;
    else
        sum = sum;

    while (sum < 12)
        sum = sum + 1;

    do
        sum = sum + 1;
    while (sum < 14);

    const int extra = 1;
    return sum + extra;
}
