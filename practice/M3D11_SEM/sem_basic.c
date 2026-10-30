#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>

void err(int ret,char *str){
    if(ret!=0){
        fprintf(stderr,"%s : %s\n",str,strerror(ret));
        exit(1);
    }   
}

sem_t sem;
void *consumer(void *arg){
    (void)arg;
    int ret=sem_wait(&sem);
    if(ret==-1){
        perror("sem_wait");
        exit(1);
    }
    printf("被唤醒,继续\n");
    pthread_exit(NULL);
}

void *producer(void *arg){
    (void)arg;
    sleep(1);
    printf("生产完成，发信号\n");
    int ret=sem_post(&sem);
    if(ret==-1){
        perror("sem_post");
        exit(1);
    }
    pthread_exit(NULL);
}

int main(){
    pthread_t pid,cid;
    int ret;

    ret=sem_init(&sem,0,0);
    if(ret==-1){
        perror("sem_init");
        exit(1);
    }

    ret=pthread_create(&pid,NULL,producer,NULL);
    err(ret,"producer create error");
    ret=pthread_create(&cid,NULL,consumer,NULL);
    err(ret,"consumer create error");

    ret=pthread_join(pid,NULL);
    err(ret,"producer join error");
    ret=pthread_join(cid,NULL);
    err(ret,"consumer join error");

    ret=sem_destroy(&sem);
    if(ret==-1){
        perror("sem_destroy");
        exit(1);
    }
    pthread_exit(NULL);
}