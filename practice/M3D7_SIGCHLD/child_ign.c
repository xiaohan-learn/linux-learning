#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(){
    pid_t pid;
    struct sigaction act;
    act.sa_handler=SIG_IGN;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    sigaction(SIGCHLD,&act,NULL);
    for(int i=0;i<10;i++){
        pid=fork();
        if(pid<0){
            perror("fork");
            exit(1);
        }
        else if(pid==0){
            printf("I'm child pid=%d\n",getpid());
            exit(i);
        }
    }
    if(pid>0){
        sleep(2);
        printf("I'm father pid=%d\n",getpid());
    }

    return 0;
}