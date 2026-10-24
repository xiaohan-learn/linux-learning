#include <unistd.h>
#include <stdio.h>
#include <signal.h>

void handler(int sig){
    (void)sig;
    printf("时间到\n");
}

int main(void){
    signal(SIGALRM,handler);
    alarm(3);
    pause();
    return 0;

}