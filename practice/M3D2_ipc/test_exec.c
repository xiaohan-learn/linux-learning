#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

int main(){
    pid_t pid=fork();
    if(pid<0){
        perror("fork");
        exit(1);
    }
    else if(pid==0){
        execlp("ls","ls","-l",NULL);
        perror("execlp");
        exit(1);
    }
    else{
        waitpid(pid,NULL,0);
        printf("I'm father pid=%d,my son pid=%d\n",getpid(),pid);
    }
    return 0;
}