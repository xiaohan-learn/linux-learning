#include <stdio.h>
#include <unistd.h>

int main(void){
    printf("进程已启动,pid=%d\n",getpid());
    printf("等待接收信号...\n");

    while(1){
        pause();
    }

    return 0;
}