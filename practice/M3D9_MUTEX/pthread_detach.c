#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void * tfn(void *arg){
    (void)arg;
    printf("thread pid=%d,tid=%lu\n",getpid(),pthread_self());
    return (void *)111;
}

int main(){
    pthread_t tid;
    int ret;
    ret=pthread_create(&tid,NULL,tfn,NULL);
    if(ret!=0){
        fprintf(stderr,"pthread_create error :%s\n",strerror(ret));
        exit(1);
    }
    ret=pthread_detach(tid);
    if(ret!=0){
        fprintf(stderr,"pthread_detach error :%s\n",strerror(ret));
        exit(1);
    }
    ret=pthread_join(tid,NULL);
    if(ret!=0){
        fprintf(stderr,"pthread_join error :%s\n",strerror(ret));
        exit(1);
    }

    return 0;
}