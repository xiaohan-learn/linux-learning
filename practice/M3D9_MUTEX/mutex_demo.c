#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

pthread_mutex_t mutex;
int cnt=0;

void * tfn(void *arg){
    (void)arg;
    int ret;
    for(int j=0;j<100000;j++){
        ret=pthread_mutex_lock(&mutex);
        if(ret!=0){
            fprintf(stderr,"lock error:%s\n",strerror(ret));
            exit(1);
        }
        cnt++;
        ret=pthread_mutex_unlock(&mutex);
        if(ret!=0){
            fprintf(stderr,"unlock error:%s\n",strerror(ret));
            exit(1);
        }
    }
    pthread_exit(NULL);
}

int main(){
    pthread_t tid[5];
    int ret;
    ret=pthread_mutex_init(&mutex,NULL);
    if(ret!=0){
        fprintf(stderr,"init error:%s\n",strerror(ret));
        exit(1);
    }

    for(int i=0;i<5;i++){
        ret=pthread_create(&tid[i],NULL,tfn,NULL);
        if(ret!=0){
            fprintf(stderr,"pthread_create error:%s\n",strerror(ret));
            exit(1);
        }
    }

    for(int i=0;i<5;i++){
        ret=pthread_join(tid[i],NULL);
        if(ret!=0){
            fprintf(stderr,"pthread_join error:%s\n",strerror(ret));
            exit(1);
        }
    }
    printf("cnt=%d,(理论为500000)\n",cnt);
    ret=pthread_mutex_destroy(&mutex);
    if(ret!=0){
        fprintf(stderr,"destory error:%s\n",strerror(ret));
        exit(1);
    }
    pthread_exit(NULL);
}