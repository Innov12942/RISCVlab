#include <stdio.h>
#include "syscall.h"
int ackermann(int m, int n)
{   
    if(n < 0 || m < 0)
        return 0;
    if(m == 0)
        return n + 1;
    if(n == 0)
        return ackermann(m - 1, 1);
    return ackermann(m - 1, ackermann(m, n - 1));
}

int main()
{
    int res = ackermann(2, 2);
    printfint(res);
    return 0;
}