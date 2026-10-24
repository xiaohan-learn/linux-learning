#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

void handler(int sig){
    (void)sig;
    printf("catch SIGINT, pid=%d\n", getpid());
    sleep(5);
    printf("SIGINT handler finished\n");
}

int main(void){
    struct sigaction act,oldact;
    act.sa_handler=handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;

    int ret=sigaction(SIGINT,&act,&oldact);
    if(ret==-1){
        perror("sigaction");
        exit(1);
    }

    while(1){
        pause();
    }
    return 0;
}