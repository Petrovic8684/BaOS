/* test9.c - switch statement
* Expected: baoc OK, exit code 20
* Tests: switch statement
*/
int main()
{
    int x = 2;
    int r = 0;

    switch (x)
    {
    case 1:
        r = 10;
        break;
    case 2:
        r = 20;
        break;
    default:
        r = 99;
        break;
    }

    return r;
}
