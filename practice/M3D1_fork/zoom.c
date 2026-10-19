#include <stdio.h>
#include <unistd.h>

int main(void){
    pid_t pid =fork();

    if(pid==0){
        printf("I'm child pid=%d,my father pid=%d\n",getpid(),getppid());
        sleep(10);
    }
    else if(pid>0){
        while(1){
        printf("I'm father pid=%d,my son=%d\n",getpid(),pid);
        sleep(1);
        }
    }
    else{
        perror("fork");
        return 1;
    }

    return 0;
}