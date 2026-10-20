#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(void){
    int ret;
    int fd[2];
    pid_t pid;

    ret=pipe(fd);
    if(ret==-1){
        perror("pipe");
        exit(1);
    }

    pid=fork();
    if(pid<0){
        perror("fork");
        exit(1);
    }
    else if(pid>0){
        close(fd[0]);
        dup2(fd[1],STDOUT_FILENO);
        close(fd[1]);
        execlp("ls","ls",NULL);
        perror("execlp");  
        wait(NULL); 
    }
    else{
        close(fd[1]);
        dup2(fd[0],STDIN_FILENO);
        close(fd[0]);
        execlp("wc","wc","-l",NULL);
        perror("execlp");
    }
    return 0;
}