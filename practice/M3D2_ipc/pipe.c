#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(void){
    int ret;
    int fd[2];
    pid_t pid;
    ssize_t rn;
    char str[]="hello,pipe\n";
    char buf[1024];

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
        write(fd[1],str,sizeof(str));
        close(fd[1]);
        wait(NULL);
    }
    else{
        close(fd[1]);
        rn=read(fd[0],buf,sizeof(buf));
        write(STDOUT_FILENO,buf,rn);
        close(fd[0]);
    }

    return 0;
}