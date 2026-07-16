/*
* test7.c - printf function call
* Expected: baoc OK, exit code 0 and print "ok"
* Tests: printf function call
*/
extern int printf(const char *fmt, ...);

int main()
{
    printf("ok\n");
    return 0;
}
