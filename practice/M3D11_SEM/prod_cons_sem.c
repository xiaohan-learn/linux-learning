#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define N 4
int buf[N];
sem_t empty;  // 信号量empty：记录空闲空位数量，初始值=N=4
sem_t full;   // 信号量full：记录已经存放了数据的位置数量，初始值=0
sem_t mutex;  // 互斥信号量mutex：保护临界区，保证同一时间只有一个线程操作缓冲区，初始值=1
int in=0;       // in：生产者的下标，标记下一个要往buf哪个位置放数据
int out=0;      // out：消费者的下标，标记下一个要从buf哪个位置取数据

void err(int ret,char *str){
    if(ret!=0){
        fprintf(stderr,"%s : %s\n",str,strerror(ret));
        exit(1);
    }
}

void *producer(void *arg){
    (void)arg;
    int ret;
    int item=0;
    while(1){
        ret=sem_wait(&empty);
        if(ret==-1){perror("producer empty wait ");     exit(1);}
        ret=sem_wait(&mutex);
        if(ret==-1){perror("producer mutex wait ");     exit(1);}

        buf[in]=item;
        int e,f,m;
        sem_getvalue(&empty,&e);
        sem_getvalue(&full,&f);
        sem_getvalue(&mutex,&m);
        printf("producer : in=%d,item=%d,empty=%d,full=%d,mutex=%d,sum=%d\n",in,item,e,f,m,e+f);

        in=(in+1)%N;
        item++;

        ret=sem_post(&mutex);
        if(ret==-1){perror("producer mutex post ");     exit(1);}
        ret=sem_post(&full);
        if(ret==-1){perror("producer full post ");     exit(1);}
        sleep(1);
    }
    pthread_exit(NULL);
}

void *consumer(void *arg){
    (void)arg;
    int ret;
    while(1){
       ret=sem_wait(&full);
       if(ret==-1){perror("consumer full wait ");     exit(1);}
       ret=sem_wait(&mutex);
       if(ret==-1){perror("comsumer mutex wait ");     exit(1);}
       
       int item=buf[out];
       int e,f,m;
       sem_getvalue(&empty,&e);
       sem_getvalue(&full,&f);
       sem_getvalue(&mutex,&m);
       printf("consumer : out=%d,item=%d,empty=%d,full=%d,mutex=%d,sum=%d\n",out,item,e,f,m,e+f);

       out=(out+1)%N;

       ret=sem_post(&mutex);
       if(ret==-1){perror("consumer mutex post ");     exit(1);}
       ret=sem_post(&empty);
       if(ret==-1){perror("consumer empty post ");     exit(1);}
       sleep(1);
    }
    pthread_exit(NULL);
}

int main(){
    pthread_t pid,cid;
    int ret;

    ret=sem_init(&empty,0,N);
    if(ret==-1){perror("main empty init ");     exit(1);}
    ret=sem_init(&full,0,0);
    if(ret==-1){perror("main full init ");     exit(1);}
    ret=sem_init(&mutex,0,1);
    if(ret==-1){perror("main mutex init ");     exit(1);}

    ret=pthread_create(&pid,NULL,producer,NULL);
    err(ret,"producer create error");
    ret=pthread_create(&cid,NULL,consumer,NULL);
    err(ret,"consumer create error");

    ret=pthread_join(pid,NULL);
    err(ret,"producer join error");
    ret=pthread_join(cid,NULL);
    err(ret,"consumer join error");

    ret=sem_destroy(&empty);
    if(ret==-1){perror("main empty destroy ");     exit(1);}
    ret=sem_destroy(&full);
    if(ret==-1){perror("main full destroy ");     exit(1);}
    ret=sem_destroy(&mutex);
    if(ret==-1){perror("main mutex destroy ");     exit(1);}

    pthread_exit(NULL);
}