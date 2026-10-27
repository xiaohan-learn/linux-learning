#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

void * tfn(void *arg){
    long i=(long) arg;
    printf("I'm %ldthread pid=%d,tid=%lu\n",i,getpid(),pthread_self());  
    return NULL; 
}

int main(void){
    int i,ret;
    pthread_t tid;
    for(i=1;i<6;i++){
        ret=pthread_create(&tid,NULL,tfn,(void *)(long)i);
        if(ret!=0){
            perror("pthread_create");
            exit(1);
        }
    }

    sleep(3);
    printf("I'm main pid=%d,tid=%lu\n",getpid(),pthread_self());
    return 0;
}