/*
 * MPSC 参考实现：N 生产者 / M 消费者 有界缓冲
 * 修复了原版 4 个致命 bug：
 *   1. in/out 在 mutex 保护下前进（多生产者/消费者共享下标，必须罩住）
 *   2. 每次循环只生产/消费 1 个 item，占 1 个槽
 *   3. 用 pthread_mutex_t 而非 sem_t 当互斥锁
 *   4. 生产者造满 ITEMS 个就退出；消费者消费完 total 个就退出
 *
 * 编译(WSL): gcc mpsc_ref.c -o mpsc_ref -lpthread && ./mpsc_ref
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define N     5    /* 缓冲区槽位数 */
#define NP    3    /* 生产者个数 */
#define NC    2    /* 消费者个数 */
#define ITEMS 10   /* 每个生产者制造的产品数 */

int buf[N];
sem_t empty, full;          /* 计数信号量：空槽 / 满槽 */
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;  /* 保护 in/out 的下标互斥锁 */

int in = 0, out = 0;        /* 多个生产者/消费者共享，必须靠 lock 罩住 */

/* 终止相关：用普通变量在 lock 下维护计数 */
int produced = 0;           /* 已生产总数 */
int consumed = 0;           /* 已消费总数 */
int total;                  /* = NP * ITEMS，全局总量 */

void err(int ret, const char *str){
    if(ret != 0){
        fprintf(stderr, "%s : %s\n", str, strerror(ret));
        exit(1);
    }
}

void *producer(void *arg){
    long id = (long)arg;
    for(int k = 0; k < ITEMS; k++){
        int item = (int)id * 100 + k;   /* 造一个带编号的 item */

        sem_wait(&empty);               /* 先等“有空槽”，再抢锁 —— 顺序不能反(Q12) */
        pthread_mutex_lock(&lock);      /* 进临界区：保护 in 和 buf[in] */

        int slot = in;
        buf[in] = item;                 /* 写当前槽 */
        in = (in + 1) % N;              /* 下标前进！多个生产者并发改 in，全靠这把锁 */
        produced++;

        printf("P%ld make item=%-4d -> buf[%d]  (in=%d produced=%d)\n",
               id, item, slot, in, produced);

        pthread_mutex_unlock(&lock);
        sem_post(&full);                /* 通知“多了一个产品” */
        usleep(1000);                   /* 仅为了让输出交错更直观，可删 */
    }
    printf("P%ld 退出（造了 %d 个）\n", id, ITEMS);
    return NULL;
}

void *consumer(void *arg){
    long id = (long)arg;
    while(1){
        sem_wait(&full);                /* 先等“有产品” */
        pthread_mutex_lock(&lock);      /* 进临界区：保护 out 和 buf[out] */

        /* 终止判断：所有产品都消费完了，多出来的唤醒信号直接退出 */
        if(consumed >= total){
            pthread_mutex_unlock(&lock);
            break;
        }

        int item = buf[out];            /* 正常取一个产品 */
        int slot = out;
        out = (out + 1) % N;            /* 下标前进！多个消费者并发改 out，靠锁 */
        consumed++;

        printf("C%ld take item=%-4d <- buf[%d]  (out=%d consumed=%d)\n",
               id, item, slot, out, consumed);

        pthread_mutex_unlock(&lock);
        sem_post(&empty);               /* 通知“多了一个空槽” */
        usleep(1000);
    }
    printf("C%ld 退出\n", id);
    return NULL;
}

int main(){
    pthread_t pid[NP], cid[NC];
    int ret;

    total = NP * ITEMS;                 /* 30 */
    sem_init(&empty, 0, N);
    sem_init(&full,  0, 0);

    for(int i = 0; i < NP; i++)
        err(pthread_create(&pid[i], NULL, producer, (void*)(long)i), "producer create");
    for(int i = 0; i < NC; i++)
        err(pthread_create(&cid[i], NULL, consumer, (void*)(long)i), "consumer create");

    for(int i = 0; i < NP; i++)
        err(pthread_join(pid[i], NULL), "producer join");
    /* 所有生产者已退出、produced==total。下面发 NC 个“唤醒信号”
     * 让可能阻塞在 sem_wait(&full) 的消费者醒来并正常退出。
     * 这些额外 post 不对应真实产品，消费者靠 consumed>=total 忽略它们。 */
    for(int i = 0; i < NC; i++)
        sem_post(&full);
    for(int i = 0; i < NC; i++)
        err(pthread_join(cid[i], NULL), "consumer join");

    sem_destroy(&empty);
    sem_destroy(&full);
    printf("全部结束：produced=%d consumed=%d\n", produced, consumed);
    return 0;
}
