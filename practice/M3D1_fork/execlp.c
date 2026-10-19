#include <stdio.h>
#include <unistd.h>

int main(void){
    pid_t pid=fork();
    if(pid==-1){
        perror("fork");
        return 1;
    }
    else if(pid==0){
        execl("/bin/ls","ls","-l",NULL);
        perror("execl");
        return 1;
    }
    else if(pid>0){
        sleep(1);
        printf("I'm father pid=%d\n",getpid());
    }
    return 0;
}