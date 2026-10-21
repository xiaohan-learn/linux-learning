#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

int main(void){
    int ret;
    int fd[2];

    ret=pipe(fd);
    if(ret<0){
        perror("pipe");
        exit(1);
    }

    pid_t pid=fork();
    if(pid<0){
        perror("fork");
        exit(1);
    }
    else if(pid==0){
        close(fd[1]);
        printf("子进程把持读端，不读数据\n");
        sleep(2);
        close(fd[0]);
        exit(0);
    }
    else{
        char buf[4096];
        int total=0;
        close(fd[0]);
        sleep(1);
        while(1){
            ssize_t wn=write(fd[1],buf,sizeof(buf));
            if(wn<0){
                perror("write");
                exit(1);
            }
            total+=wn;
            printf("已写入%d字节\n",total);
        }
        close(fd[1]);
        wait(NULL);
    }
    return 0;
}