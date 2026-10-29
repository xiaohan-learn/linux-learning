#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

pthread_mutex_t mutexA,mutexB;
void *t1(void *arg){
    (void)arg;
    int ret=pthread_mutex_lock(&mutexA);
    if(ret!=0){
        fprintf(stderr,"t1 lockA error:%s\n",strerror(ret));
        exit(1);
    }
    printf("thread1 : t1 get A\n"); fflush(stdout);
    sleep(1);
    ret=pthread_mutex_lock(&mutexB);
    if(ret!=0){
        fprintf(stderr,"t1 lockB error:%s\n",strerror(ret));
        exit(1);
    }
    printf("thread1 : t1 get B\n"); fflush(stdout);

    ret=pthread_mutex_unlock(&mutexA);
    if(ret!=0){
        fprintf(stderr,"t1 unlockA error:%s\n",strerror(ret));
        exit(1);
    }
    ret=pthread_mutex_unlock(&mutexB);
    if(ret!=0){
        fprintf(stderr,"t1 unlockB error:%s\n",strerror(ret));
        exit(1);
    }
    pthread_exit(NULL);
}

void *t2(void *arg){
    (void)arg;
    int ret=pthread_mutex_lock(&mutexB);
    if(ret!=0){
        fprintf(stderr,"t2 lockB error:%s\n",strerror(ret));
        exit(1);
    }
    printf("thread2 : t2 get B\n"); fflush(stdout);
    sleep(1);
    ret=pthread_mutex_lock(&mutexA);
    if(ret!=0){
        fprintf(stderr,"t2 lockA error:%s\n",strerror(ret));
        exit(1);
    }
    printf("thread2 : t2 get A\n"); fflush(stdout);

    ret=pthread_mutex_unlock(&mutexB);
    if(ret!=0){
        fprintf(stderr,"t2 unlockB error:%s\n",strerror(ret));
        exit(1);
    }
    ret=pthread_mutex_unlock(&mutexA);
    if(ret!=0){
        fprintf(stderr,"t2 unlockA error:%s\n",strerror(ret));
        exit(1);
    }
    pthread_exit(NULL);
}

int main(){
    pthread_t tid1,tid2;
    int ret;
    //初始化锁
    ret=pthread_mutex_init(&mutexA,NULL);
    if(ret!=0){
        fprintf(stderr,"main initA error:%s\n",strerror(ret));
        exit(1);
    }
    ret=pthread_mutex_init(&mutexB,NULL);
    if(ret!=0){
        fprintf(stderr,"main initB error:%s\n",strerror(ret));
        exit(1);
    }
    //创建线程
    ret=pthread_create(&tid1,NULL,t1,NULL);
    if(ret!=0){
        fprintf(stderr,"main create1 error:%s\n",strerror(ret));
        exit(1);
    }
    ret=pthread_create(&tid2,NULL,t2,NULL);
    if(ret!=0){
        fprintf(stderr,"main create2 error:%s\n",strerror(ret));
        exit(1);
    }
    //阻塞回收线程
    ret=pthread_join(tid1,NULL);
    if(ret!=0){
        fprintf(stderr,"main join1 error:%s\n",strerror(ret));
        exit(1);
    }
    ret=pthread_join(tid2,NULL);
    if(ret!=0){
        fprintf(stderr,"main join2 error:%s\n",strerror(ret));
        exit(1);
    }
    //销毁锁
    ret=pthread_mutex_destroy(&mutexA);
    if(ret!=0){
        fprintf(stderr,"main destoryA error:%s\n",strerror(ret));
        exit(1);
    }
    ret=pthread_mutex_destroy(&mutexB);
    if(ret!=0){
        fprintf(stderr,"main destoryB error:%s\n",strerror(ret));
        exit(1);
    }
    pthread_exit(NULL);

}