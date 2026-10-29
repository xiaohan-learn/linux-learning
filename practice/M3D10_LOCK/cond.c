#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond=PTHREAD_COND_INITIALIZER;
int ready=0;   //0：没有产品，消费者需要等待；1：产品就绪，消费者可以消费。

void *consumer(void *arg){
    int ret;
    long i=(long)arg;

    ret=pthread_mutex_lock(&mutex);
    if(ret!=0){
        fprintf(stderr,"consumer lock error:%s\n",strerror(ret));
        exit(1);
    }
    while(ready==0){
        printf("没有产品，消费者%ld需要等待,ready=%d\n",i,ready);
        ret=pthread_cond_wait(&cond,&mutex);
        if(ret!=0){
        fprintf(stderr,"consumer wait error:%s\n",strerror(ret));
        exit(1);
    }
    }
    printf("产品就绪，消费者%ld可以消费,ready=%d\n",i,ready);
    ready=0;

    ret=pthread_mutex_unlock(&mutex);
    if(ret!=0){
        fprintf(stderr,"consumer unlock error:%s\n",strerror(ret));
        exit(1);
    }
    pthread_exit(NULL);
}

void *product(void *arg){
    int ret;
    long i=(long)arg;
    sleep(1);

    ret=pthread_mutex_lock(&mutex);
    if(ret!=0){
        fprintf(stderr,"product lock error:%s\n",strerror(ret));
        exit(1);
    }
    ready=1;
    printf("生产者%ld产品就绪,发送signal信号,ready=%d\n",i,ready);

    ret=pthread_cond_signal(&cond);
    if(ret!=0){
        fprintf(stderr,"product signal error:%s\n",strerror(ret));
        exit(1);
    }

    ret=pthread_mutex_unlock(&mutex);
    if(ret!=0){
        fprintf(stderr,"product unlock error:%s\n",strerror(ret));
        exit(1);
    }
    pthread_exit(NULL);
}

int main (){
    pthread_t ctid,ptid;
    int ret;
    ret=pthread_create(&ctid,NULL,consumer,(void *)(long)1);
    if(ret!=0){
        fprintf(stderr,"consumer create error:%s\n",strerror(ret));
        exit(1);
    }
    ret=pthread_create(&ptid,NULL,product,(void *)(long)2);
    if(ret!=0){
        fprintf(stderr,"product create error:%s\n",strerror(ret));
        exit(1);
    }

    ret=pthread_join(ctid,NULL);
    if(ret!=0){
        fprintf(stderr,"consumer join error:%s\n",strerror(ret));
        exit(1);
    }
    ret=pthread_join(ptid,NULL);
    if(ret!=0){
        fprintf(stderr,"product join error:%s\n",strerror(ret));
        exit(1);
    }

    ret=pthread_mutex_destroy(&mutex);
    if(ret!=0){
        fprintf(stderr,"mutex_destory error:%s\n",strerror(ret));
        exit(1);
    }
    ret=pthread_cond_destroy(&cond);
    if(ret!=0){
        fprintf(stderr,"cond_destroy error:%s\n",strerror(ret));
        exit(1);
    }

    pthread_exit(NULL);
}