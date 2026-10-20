#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>

int main(void){
    pid_t pid=fork();
    if(pid<0){
        perror("fork");
        exit(1);
    }
    else if(pid==0){
        printf("I'm child pid=%d\n",getpid());
        sleep(2);
        exit(7);
    }
    else {
        pid_t ret;
        int status;
        while(1){
            ret=waitpid(pid,&status,WNOHANG);
            if(ret==-1){
                perror("waitpid");
                break;
            }
            else if(ret==0){
            printf("子进程还在运行\n");
            sleep(1);
            continue;
            }
            else {
                printf("回收成功,child pid=%d\n",pid);
                if(WIFEXITED(status)){
                    printf("子进程正常退出, 退出码=%d\n",WEXITSTATUS(status));
                }
                break;
            }
        }
    }
   return 0; 
}