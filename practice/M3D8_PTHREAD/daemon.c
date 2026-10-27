#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

void creat_daemon(void){
    pid_t pid;
    pid=fork();
    if(pid<0){
        perror("fork");
        exit(1);
    }
    else if(pid>0){
        exit(0);
    }
    else{
        setsid();
        chdir("/");
        umask(0);
        int fd=open("/dev/null",O_RDWR);
        if(fd<0){
            perror("open");
            exit(1);
        }
        dup2(fd,STDIN_FILENO);
        dup2(fd,STDOUT_FILENO);
        dup2(fd,STDERR_FILENO);
        close (fd);
    }
}

int main(void){
    creat_daemon();
    while(1){
        sleep(2);
    }
    return 0;
}