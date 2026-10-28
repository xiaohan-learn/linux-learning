#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

void *tfn1(void *arg){
    (void)arg;
    printf("tfn1 pthread pid=%d,tid1=%lu\n",getpid(),pthread_self());
    return (void *)111;
}
void *tfn2(void *arg){
    (void)arg;
    printf("tfn2 pthread pid=%d,tid2=%lu\n",getpid(),pthread_self());
    pthread_exit((void *)222);
}
void *tfn3(void *arg){
    (void)arg;
    while(1){
        printf("tfn3 pthread pid=%d,tid3=%lu\n",getpid(),pthread_self());
        sleep(1);
    }
    return (void *)333;
}

int main(){
    pthread_t tid[3];
    int ret;
    void *retn;

    ret=pthread_create(&tid[0],NULL,tfn1,NULL);
    if(ret!=0){
        fprintf(stderr,"tfn1 pthread_creat error:%s\n",strerror(ret));
        exit(1);
    }
    pthread_join(tid[0],(void **)&retn);
    printf("tfn1 main return code=%ld\n\n",(long)retn);

    ret=pthread_create(&tid[1],NULL,tfn2,NULL);
    if(ret!=0){
        fprintf(stderr,"tfn2 pthread_creat error:%s\n",strerror(ret));
        exit(1);
    }
    pthread_join(tid[1],(void **)&retn);
    printf("tfn2 main return code=%ld\n\n",(long)retn);

    ret=pthread_create(&tid[2],NULL,tfn3,NULL);
    if(ret!=0){
        fprintf(stderr,"tfn3 pthread_creat error:%s\n",strerror(ret));
        exit(1);
    }
    sleep(3);
    pthread_cancel(tid[2]);
    pthread_join(tid[2],(void **)&retn);
    printf("tfn3 main return code=%ld\n",(long)retn);

    pthread_exit(NULL);

}