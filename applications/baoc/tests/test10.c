/* test10.c - multi-file compilation
* Expected: baoc OK, exit code 7
* Tests: multi-file compilation
*/
extern int helper(int x);

int main()
{
    return helper(4);
}
