#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

void *more(void *arg){
    long i=(long )arg;
    printf("%ld thread pid=%d,tid=%lu\n",i,getpid(),pthread_self());
    return (void *)(i*10);
}

int main(void){
    pthread_t tid[5];
    int i,ret;
    void *res;
    for(i=1;i<6;i++){
        ret=pthread_create(&tid[i],NULL,more,(void *)(long)i);
        if(ret!=0){
            perror("pthread_create");
            exit(1);
        }
        pthread_join(tid[i],(void **)&res);
        printf("%d return code=%ld\n",i,(long)res);
    }
    printf("main pid=%d,tid=%lu\n",getpid(),pthread_self());
    pthread_exit(NULL);  
}