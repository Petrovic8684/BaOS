#include <stdio.h>
#include <time.h>

int main(void)
{
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);

    printf("Current date: %s", asctime(lt));

    return 0;
}
