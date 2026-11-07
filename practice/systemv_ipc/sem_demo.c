#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

union sem_un{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

void P(int semid){
    struct sembuf op;
    op.sem_num=0;
    op.sem_op=-1;
    op.sem_flg=0;
    int ret=semop(semid,&op,1);
    if(ret==-1){
        perror("P semop");
        return;
    }
}

void V(int semid){
    struct sembuf op;
    op.sem_num=0;
    op.sem_op=+1;
    op.sem_flg=0;
    int ret=semop(semid,&op,1);
    if(ret==-1){
        perror("V semop");
        return;
    }
}

int main(){
    key_t sem_key=ftok(".",100);
    key_t shm_key=ftok(".",200);

    int shmid=shmget(shm_key,1024,IPC_CREAT | 0664);
    if(shmid==-1){
        perror("shmget");
        exit(1);
    }
    char *s=(char *)shmat(shmid,NULL,0);
    if(s==(void *)-1){
        perror("shmat");
        exit(1);
    }

    int semid=semget(sem_key,1,IPC_CREAT | 0664);
    if(semid==-1){
        perror("semget");
        exit(1);
    }

    union sem_un un;
    un.val=1;
    int ret=semctl(semid,0,SETVAL,un);
    if(ret==-1){
        perror("setval semctl");
        exit(1);
    }

    pid_t pid=fork();
    if(pid<0){
        perror("fork");
        exit(1);
    }
    else if(pid==0){
        sleep(1);
        P(semid);
        printf("child read : %s\n",s);
        V(semid);
        ret=shmdt(s);
        if(ret==-1){
            perror("child shmdt");
            shmctl(shmid,IPC_RMID,NULL);
            semctl(semid,0,IPC_RMID,0);
            exit(1);
        }   
    }
    else{
        P(semid);
        strcpy(s,"hello sem");
        printf("parent write success\n"); fflush(stdout);
        V(semid);
        wait(NULL);
        ret=shmdt(s);
        if(ret==-1){
            perror(" parent shmdt");
            shmctl(shmid,IPC_RMID,NULL);
            semctl(semid,0,IPC_RMID,0);
            exit(1);
        }
        shmctl(shmid,IPC_RMID,NULL);
        semctl(semid,0,IPC_RMID,0);
    }
    return 0;
}