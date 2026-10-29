#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

pthread_mutex_t mutex;
void * tryfunc (void *arg){
    (void)arg;
    int ret=pthread_mutex_trylock(&mutex);
    if(ret==0){
        printf("try lock success\n");
        pthread_mutex_unlock(&mutex);
    }
    else if(ret==EBUSY){
        printf("trylock fail,锁被占用(%d)\n",ret);
    }
    else{
        fprintf(stderr,"trylock error:%s\n",strerror(ret));
        exit(1);
    }
    pthread_exit(NULL);
}

int main(){
    pthread_t tid1,tid2;
    int ret;
    ret=pthread_mutex_init(&mutex,NULL);
    if(ret!=0){
        fprintf(stderr,"init error:%s\n",strerror(ret));
        exit(1);
    }

    ret=pthread_mutex_lock(&mutex);
    if(ret!=0){
        fprintf(stderr,"lock error:%s\n",strerror(ret));
        exit(1);
    }

    ret=pthread_create(&tid1,NULL,tryfunc,NULL);
    if(ret!=0){
        fprintf(stderr,"create1 error:%s\n",strerror(ret));
        exit(1);
    }

    ret=pthread_join(tid1,NULL);
    if(ret!=0){
        fprintf(stderr,"join1 error:%s\n",strerror(ret));
        exit(1);
    }
    printf("main1:hello,trylock\n");

    ret=pthread_mutex_unlock(&mutex);
    if(ret!=0){
        fprintf(stderr,"unlock error:%s\n",strerror(ret));
        exit(1);
    }

    printf("\n========锁空闲,再次trylock========\n");
    ret=pthread_create(&tid2,NULL,tryfunc,NULL);
    if(ret!=0){
        fprintf(stderr,"create2 error:%s\n",strerror(ret));
        exit(1);
    }

    ret=pthread_join(tid2,NULL);
    if(ret!=0){
        fprintf(stderr,"join2 error:%s\n",strerror(ret));
        exit(1);
    }
    printf("main2:hello,trylock\n");

    ret=pthread_mutex_destroy(&mutex);
    if(ret!=0){
        fprintf(stderr,"destory error:%s\n",strerror(ret));
        exit(1);
    }
    pthread_exit(NULL);
}