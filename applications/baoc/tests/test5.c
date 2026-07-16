/* test5.c - intentionally unsupported: float/double
 * Expected: baoc FAIL at compile time
 */
int main()
{
    float x = 1.0f;
    double y = 2.0;
    return (int)(x + y);
}
