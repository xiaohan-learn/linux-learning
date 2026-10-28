#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

int cnt=0;

void * tfn(void *arg){
    (void)arg;
    for(int j=0;j<100000;j++){
        cnt++;
    }
    pthread_exit(NULL);
}

int main(){
    pthread_t tid[5];
    int i,ret;
    for(i=0;i<5;i++){
        ret=pthread_create(&tid[i],NULL,tfn,NULL);
        if(ret!=0){
            fprintf(stderr,"pthread_create error:%s\n",strerror(ret));
            exit(1);
        }
    }
    
    for(i=0;i<5;i++){
        ret=pthread_join(tid[i],NULL);
        if(ret!=0){
            fprintf(stderr,"pthread_join error:%s\n",strerror(ret));
            exit(1);
        }
    }
    printf("cnt=%d,(理论应500000) \n",cnt);
    pthread_exit(NULL);
}