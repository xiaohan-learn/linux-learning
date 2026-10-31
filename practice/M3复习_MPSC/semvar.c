#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define N 5     /* 缓冲区槽位数 */
#define NP 3    /* 生产者个数 */
#define NC 2    /* 消费者个数 */
#define ITEMS 10    /* 每个生产者制造的产品数 */

int buf[N];
sem_t empty;     /* 计数信号量：空槽个数 */
sem_t full;      /* 计数信号量：满槽个数 */
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;    /* 保护 in/out 的下标互斥锁 */
int in=0;   // in：生产者的下标，标记下一个要往buf哪个位置放数据
int out=0;  // out：消费者的下标，标记下一个要往buf哪个位置取数据
int produced=0;     /* 已生产总数 */
int consumed=0;     /* 已消费总数 */
int total;       /* = NP * ITEMS，全局总量 */

void err(int ret,char *str){
    if(ret!=0){
        fprintf(stderr,"%s : %s\n",str,strerror(ret));
        exit(1);
    }
}

void *producer(void *arg){
    long i=(long)arg;
    int k,ret;

    for(k=0;k<ITEMS;k++){
        int item=(long)i*100+k;     /* 造一个带编号的 item */
        ret=sem_wait(&empty);        /* 先等“有空槽”，再抢锁 */
        if(ret==-1){perror("producer empty wait error");     exit(1);}
        ret=pthread_mutex_lock(&mutex);       /* 进临界区：保护 in 和 buf[in] */
        err(ret,"producer mutex_lock error");

        buf[in]=item;   /* 写当前槽 */
        in=(in+1)%N;     /* 下标前进！多个生产者并发改 in，全靠这把锁 */
        produced++;
        int e,f;
        sem_getvalue(&empty,&e);
        sem_getvalue(&full,&f);
        printf("producer%ld,in=%d,item=%d,empty=%d,full=%d,sum=%d\n",i,in,item,e,f,e+f);

        ret=pthread_mutex_unlock(&mutex);
        err(ret,"producer mutex unlock");
        ret=sem_post(&full);    /* 通知“多了一个产品” */
        if(ret==-1){perror("producer full post error");     exit(1);}
        sleep(1);
    }
    printf("producer%ld return\n",i);
    return NULL;
}

void *consumer(void *arg){
    long i=(long)arg;
    int ret;
    while(1){
        ret=sem_wait(&full);      /* 先等“有产品” */
        if(ret==-1){perror("consumer full wait error");     exit(1);}
        ret=pthread_mutex_lock(&mutex);     /* 进临界区：保护 out 和 buf[out] */
        err(ret,"consumer mutex_lock error");

        if(consumed>=total){    /* 终止判断：所有产品都消费完了，多出来的唤醒信号直接退出 */
            ret=pthread_mutex_unlock(&mutex);
            err(ret,"consumer mutex unlock");
            break;
        }

        int item=buf[out];       /* 正常取一个产品 */
        int e,f;
        sem_getvalue(&empty,&e);
        sem_getvalue(&full,&f);
        printf("consumer%ld,out=%d,item=%d,empty=%d,full=%d,sum=%d\n",i,out,item,e,f,e+f);
        out=(out+1)%N;    /* 下标前进！多个消费者并发改 out，靠锁 */
        consumed++;

        ret=pthread_mutex_unlock(&mutex);
        err(ret,"consumer mutex unlock");
        ret=sem_post(&empty);    /* 通知“多了一个空槽” */
        if(ret==-1){perror("consumer empty post error");     exit(1);}
        sleep(1);
    }
    return NULL;
}

int main(){
    pthread_t pid[NP],cid[NC];
    int i,ret;
    total=ITEMS*NP;      /* 30 */

    ret=sem_init(&empty,0,N);
    if(ret==-1){perror("empty init error");     exit(1);}
    ret=sem_init(&full,0,0);
    if(ret==-1){perror("full init error");     exit(1);}

    for(i=0;i<NP;i++){
        ret=pthread_create(&pid[i],NULL,producer,(void*)(long)i);
        err(ret,"producer create error");
    }
    for(i=0;i<NC;i++){
        ret=pthread_create(&cid[i],NULL,consumer,(void *)(long)i);
        err(ret,"consumer create error");
    }

    for(i=0;i<NP;i++){
        ret=pthread_join(pid[i],NULL);
        err(ret,"producer join error");
    }
    
     /* 所有生产者已退出、produced==total。下面发 NC 个“唤醒信号”
     * 让可能阻塞在 sem_wait(&full) 的消费者醒来并正常退出。
     * 这些额外 post 不对应真实产品，消费者靠 consumed>=total 忽略它们。 */
    for(i=0;i<NC;i++){
        ret=sem_post(&full);
        if(ret==-1){perror("consumer full post error");     exit(1);}
    }
    for(i=0;i<NC;i++){
        ret=pthread_join(cid[i],NULL);
        err(ret,"consumer join error");
    }

    ret=sem_destroy(&empty);
    if(ret==-1){perror("empty destroy error");     exit(1);}
    ret=sem_destroy(&full);
    if(ret==-1){perror("full destroy error");     exit(1);}
    ret=pthread_mutex_destroy(&mutex);
    err(ret,"mutex_destroy error");
    
    pthread_exit(NULL);
}