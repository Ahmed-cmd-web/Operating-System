#include <stdio.h>

void ad(int a, int b)
{
    printf("%d\n", a + b);
}

int main()
{
    void (*operations[3])(int, int) = {ad};
    return 0;
}