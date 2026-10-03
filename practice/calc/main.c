#include <stdio.h>
#include "calc.h"
int main(){
    int a=add(3,4);
    int b=sub(7,5);
    printf("3+4=%d\n",a);
    printf("7-5=%d\n",b);
    return 0;
}