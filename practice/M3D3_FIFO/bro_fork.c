#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc,char **argv){
    int n=(argc>1) ? atoi(argv[1]) : 2;
    int ret,i;
    int fd[2];
    pid_t pid[2];

    ret=pipe(fd);
    if(ret==-1){
        perror("pipe");
        exit(1);
        }

    for(i=0 ;i<n;i++){
        pid[i]=fork();
        if(pid[i]<0){
            perror("fork");
            exit(1);
        }
        else if(pid[i]==0){
            break;
        }
    }

    if(i==n){
        close(fd[0]);
        close(fd[1]);
        wait(NULL);
        wait(NULL);
    }
    else if(i==0){  //兄
        close(fd[0]);
        dup2(fd[1],STDOUT_FILENO);
        close(fd[1]);
        execlp("ls","ls",NULL);
        perror("execlp");
        exit(1);
    }
    else if(i==1){  //弟
        close(fd[1]);
        dup2(fd[0],STDIN_FILENO);
        close(fd[0]);
        execlp("wc","wc","-l",NULL);
        perror("execlp");
        exit(1);
    }
    return 0;
}