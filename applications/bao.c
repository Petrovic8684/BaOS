#include <stdio.h>

void main(void)
{
    clear();

    char buf[100];
    printf("Your name: ");
    scanf("%s", buf);

    printf("Hello, %s.", buf);

    return;
}
