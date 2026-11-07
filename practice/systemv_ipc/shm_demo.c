#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/wait.h>

int main(){
    key_t key=ftok(".",101);
    int shmid=shmget(key,1024,IPC_CREAT | 0664);
    if(shmid==-1){
        perror("shmget");
        exit(1);
    }

    char *p=(char *)shmat(shmid,NULL,0);
    if(p==(void *)-1){
        perror("shmat");
        exit(1);
    }

    int ret;
    pid_t pid=fork();
    if(pid<0){
        perror("fork");
        exit(1);
    }
    else if(pid==0){
        strcpy(p,"hello shm");
        printf("child write succes\n"); fflush(stdout);
        ret=shmdt(p);
        if(ret==-1){
            perror("shmdt");
            ret=shmctl(shmid,IPC_RMID,NULL);
            exit(1);
        }
    }
    else{
        sleep(1);
        printf("parent read: %s\n",p);
        ret=shmdt(p);
        if(ret==-1){
            perror("shmdt");
            ret=shmctl(shmid,IPC_RMID,NULL);
            exit(1);
        }
        wait(NULL);
        ret=shmctl(shmid,IPC_RMID,NULL);
        if(ret==-1){
            perror("shmctl");
            exit(1);
        }
    }
    return 0;

}