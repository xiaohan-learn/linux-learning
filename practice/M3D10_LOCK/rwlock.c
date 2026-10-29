#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

pthread_rwlock_t lock;
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
int readers_in=0;

void *read_func(void *arg){
    long i=(long)arg;
    int ret=pthread_rwlock_rdlock(&lock);
    if(ret!=0){
        fprintf(stderr,"rwlock_rdlock error:%s\n",strerror(ret));
        exit(1);
    }
    ret=pthread_mutex_lock(&mutex); //保护计数器
    if(ret!=0){
        fprintf(stderr,"mutex_lock1 error:%s\n",strerror(ret));
        exit(1);
    }
    readers_in++;
    printf("读线程%ld拿到锁,当前readers_in=%d\n",i,readers_in);
    if(readers_in==3){
        printf("读者同时进入\n");
    }
    ret=pthread_mutex_unlock(&mutex);
    if(ret!=0){
        fprintf(stderr,"mutex_unlock1 error:%s\n",strerror(ret));
        exit(1);
    }
    sleep(1);

    ret=pthread_mutex_lock(&mutex);
    if(ret!=0){
        fprintf(stderr,"mutex_lock2 error:%s\n",strerror(ret));
        exit(1);
    }
    readers_in--;
    ret=pthread_mutex_unlock(&mutex);
    if(ret!=0){
        fprintf(stderr,"mutex_unlock2 error:%s\n",strerror(ret));
        exit(1);
    }
    ret=pthread_rwlock_unlock(&lock);
    if(ret!=0){
        fprintf(stderr,"read rwlock_unlock error:%s\n",strerror(ret));
        exit(1);
    }
    pthread_exit(NULL);
}

void *write_func(void *arg){
    long i=(long)arg;
    int ret=pthread_rwlock_wrlock(&lock);
    if(ret!=0){
        fprintf(stderr,"rwlock_wrlock error:%s\n",strerror(ret));
        exit(1);
    }
    printf("写线程%ld拿到锁,当前readers_in=%d\n",i,readers_in);
    sleep(1);
    ret=pthread_rwlock_unlock(&lock);
    if(ret!=0){
        fprintf(stderr,"write rwlock_unlock error:%s\n",strerror(ret));
        exit(1);
    }
    pthread_exit(NULL);
}

int main(){
    pthread_t tid[4];
    int i,ret;
    ret=pthread_rwlock_init(&lock,NULL);
    if(ret!=0){
        fprintf(stderr,"rwlock_init error:%s\n",strerror(ret));
        exit(1);
    }

    for(i=0;i<3;i++){
        ret=pthread_create(&tid[i],NULL,read_func,(void *)(long)i);
        if(ret!=0){
        fprintf(stderr,"read create error:%s\n",strerror(ret));
        exit(1);
        }
    }
    sleep(1);
    i=3;
    ret=pthread_create(&tid[3],NULL,write_func,(void *)(long)i);
    if(ret!=0){
        fprintf(stderr,"write create error:%s\n",strerror(ret));
        exit(1);
    }

    for(i=0;i<4;i++){
        ret=pthread_join(tid[i],NULL);
        if(ret!=0){
        fprintf(stderr,"join error:%s\n",strerror(ret));
        exit(1);
        }
    }
    ret=pthread_mutex_destroy(&mutex);
    if(ret!=0){
        fprintf(stderr,"mutex_destroy error:%s\n",strerror(ret));
        exit(1);
    }
    ret=pthread_rwlock_destroy(&lock);
    if(ret!=0){
        fprintf(stderr,"rwlock_destroy error:%s\n",strerror(ret));
        exit(1);
    }
    pthread_exit(NULL);
}
