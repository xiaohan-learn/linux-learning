#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc,char **argv){
    int n=(argc>1) ? atoi(argv[1]) : 5; 
    for(int i=0;i<n;i++){
        pid_t pid=fork();
        if(pid<0){
            perror("fork");
            exit(1);
        }
        else if(pid==0){
            printf("child %d ,child pid=%d ,father pid=%d\n",i,getpid(),getppid());
            exit(0);
        }
        printf("father creat child %d ,father pid=%d ,child pid=%d\n",i,getpid(),pid);
        wait(NULL);
    }
    return 0;
}