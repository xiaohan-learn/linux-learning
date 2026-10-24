#include <unistd.h>
#include <stdio.h>

int main(void){
    int count=0;
    alarm(1);
    while(1){
        count++;
        printf("%d\n",count);
    }
    return 0;
}