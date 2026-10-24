#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

void handler(int sig){
    printf("child got SIGUSR1\n");
}

int main(void){
    pid_t pid =fork();
    if(pid<0){
        perror("fork");
        exit(1);
    }
    else if(pid==0){
        signal(SIGUSR1,handler);
        while(1){
            pause();
        }
    }
    else{
        sleep(1);
        kill(pid,SIGUSR1);
        sleep(1);
        kill(pid,SIGKILL);
        wait(NULL);
    }
    return 0;
}