#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void handler(int sig){
    if(sig==SIGINT){
        printf("catch SIGINT ,PID=%d\n",getpid());
        sleep(10);
    }
}

int main(){
    struct sigaction act;
    act.sa_handler=handler;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask,SIGQUIT);
    act.sa_flags=0;

    int ret=sigaction(SIGINT,&act,NULL);
    if(ret==-1){
        perror("sigaction");
        exit(1);
    }

    while(1){
        pause();
    }
    return 0;

}