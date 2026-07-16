/* test12.c - array initialization
* Expected: baoc OK, exit code 6
* Tests: array initialization
*/
int main()
{
    int a[3] = {1, 2, 3};
    int s = 0;
    int i = 0;
    while (i < 3)
    {
        s = s + a[i];
        i = i + 1;
    }
    return s;
}
