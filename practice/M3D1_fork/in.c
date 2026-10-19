#include <stdio.h>
#include <unistd.h>

int main(void){
    pid_t pid =fork();

    if(pid==0){      //子进程成为孤儿进程
        while (1){
        printf("I'm child pid=%d,my father pid=%d\n",getpid(),getppid());
        sleep(1);
        }
    }
    else if(pid>0){    //父进程死亡
        printf("I'm father pid=%d,my son=%d\n",getpid(),pid);
        sleep(5);
    }
    else{
        perror("fork");
        return 1;
    }

    return 0;
}