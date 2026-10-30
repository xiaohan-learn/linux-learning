#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

sem_t limit;

void err(int ret,char *str){
    if(ret!=0){
        fprintf(stderr,"%s : %s\n",str,strerror(ret));
        exit(1);
    }
}

void *worker(void *arg){
    long i=(long)arg;
    int ret;

    ret=sem_wait(&limit);
    if(ret==-1){perror("sem_wait");   exit(1);}
    printf("线程%ld已进入\n",i);    fflush(stdout);
    sleep(2);
    printf("线程%ld已离开\n",i);    fflush(stdout);

    ret=sem_post(&limit);
    if(ret==-1){perror("sem_post");   exit(1);}
    pthread_exit(NULL);
}

int main(){
    pthread_t tid[5];
    int i,ret;

    ret=sem_init(&limit,0,3);
    if(ret==-1){perror("sem_init error");  exit(1);}

    for(i=0;i<5;i++){
        ret=pthread_create(&tid[i],NULL,worker,(void *)(long)i);
        err(ret,"pthread_create error");
    }

    for(i=0;i<5;i++){
        ret=pthread_join(tid[i],NULL);
        err(ret,"pthread_join error");
    }

    ret=sem_destroy(&limit);
    if(ret!=0){perror("sem_destroy");   exit(1);}
    pthread_exit(NULL);
}