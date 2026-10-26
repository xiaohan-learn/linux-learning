#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

void re_handler(int sig){
    (void)sig;
    int status;
    pid_t wpid;
    while((wpid=waitpid(-1,&status,WNOHANG))>0){
        if(WIFEXITED(status))
        printf("recycle child pid=%d, exit code=%d\n",wpid,WEXITSTATUS(status));
    }
}

int main(void){
    pid_t pid;
    int i;

    struct sigaction act;
        act.sa_handler=re_handler;
        sigemptyset(&act.sa_mask);
        act.sa_flags=0;
        sigaction(SIGCHLD,&act,NULL);

    for(i=0;i<10;i++){
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
        printf("I'm father pid=%d\n",getpid());
        while (1){
            pause();
        }
    }
    return 0;
}