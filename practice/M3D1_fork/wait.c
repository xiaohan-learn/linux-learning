#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void){
    pid_t wpid;
    int status;
    pid_t pid =fork();

    if(pid==0){
        printf("I'm child pid=%d,my father pid=%d\n",getpid(),getppid());
        sleep(5);
        printf("======child die======\n");
        return 7;
    }
    else if(pid>0){
       wpid=wait(&status); //如子进程未终止，父进程阻塞在这个函数上
       if(wpid==-1){
        perror("wait");
        return 1;
       }
       if(WIFEXITED(status)){    //子进程正常退出，返回值7
        printf("child return=%d\n",WEXITSTATUS(status));
       }
       if(WIFSIGNALED(status)){   //子进程被信号终止，返回值被kill信号编号
        printf("child kill single=%d\n",WTERMSIG(status));
       }
    }
    else{
        perror("fork");
        return 1;
    }

    return 0;
}