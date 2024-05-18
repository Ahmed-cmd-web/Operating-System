#include <stdio.h>




int main()
{
    int arr[5];
    arr[0] = 1;

    printf("%d\n", sizeof(arr) / sizeof(arr[0]));
    /* code */
    return 0;
}
