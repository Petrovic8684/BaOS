/* test8.c - globals
* Expected: baoc OK, exit code 11
* Tests: static local variable
*/
static int counter = 0;
int bump;

int main()
{
    counter = counter + 1;
    bump = 10;
    return counter + bump;
}
