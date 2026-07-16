/* test3.c - functions, pointers, arrays
 * Expected: baoc OK, exit code 9  (add(4, 5))
 * Tests: user functions, local arrays, pointer indexing, call/return
 * Note: pointers/arrays are still rough in baoc — watch for edge cases.
 */
int add(int a, int b)
{
    return a + b;
}

int main()
{
    int arr[3];
    int *p;
    int i;

    arr[0] = 1;
    arr[1] = 4;
    arr[2] = 9;

    p = arr;
    i = p[1];

    return add(i, 5);
}
