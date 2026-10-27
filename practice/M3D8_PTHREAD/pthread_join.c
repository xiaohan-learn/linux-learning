#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

void *join(void *arg){
    return (void *)123;
}

int main(void){
    int ret;
    pthread_t tid;
    void *res;
    ret=pthread_create(&tid,NULL,join,NULL);
    if(ret!=0){
        perror("pthread");
        exit(1);
    }

    pthread_join(tid,(void **)&res);
    printf("child thread return code =%ld\n",(long)res);
    pthread_exit(NULL);
}