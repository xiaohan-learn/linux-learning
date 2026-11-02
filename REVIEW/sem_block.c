/*
 * sem_block.c —— 信号量版「采集-处理」阻塞队列
 *
 * 与同目录 cond_block.c 是同一个问题的两套原语实现，可逐行对照：
 *   - 本文件(信号量)：empty(N) / full(0) / pthread_mutex_t(1)，计数由信号量自带
 *   - cond_block.c  ：mutex(1) + count + not_empty/not_full，计数自己维护
 *
 * 关键区别（今天这题的核心）：
 *   信号量无虚假唤醒 -> sem_wait 不需要 while 包裹；
 *   condvar  有虚假唤醒 -> pthread_cond_wait 必须 while 复查条件。
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#define N 5
int buf[N];
sem_t empty;                              /* 空槽计数，初值 N */
sem_t full;                               /* 产品计数，初值 0 */
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;   /* 保护 buf/in/out 临界区 */
int in=0,out=0;
int frame_id=1;                           /* 帧编号，只有生产者碰 */
int consumed=0;                           /* 消费计数，只有消费者碰 */
void err(int ret,char *str){
if(ret!=0){
fprintf(stderr,"%s : %s\n",str,strerror(ret));
exit(1);
}
}
void *prod(void *arg){
(void)arg;
int ret;
while(frame_id<=10){
/* ① 等一个空槽：信号量无虚假唤醒，不需要 while */
ret=sem_wait(&empty);
if(ret==-1){
perror("prod empty wait error");
exit(1);
}
/* ② 临界区：写槽位、推进下标 */
ret=pthread_mutex_lock(&mutex);
err(ret,"prod lock error");
buf[in]=frame_id;
in=(in+1)%N;
int e,f;
sem_getvalue(&empty,&e);
sem_getvalue(&full,&f);
printf("PROD:in=%d,frame_id=%d,empty=%d,full=%d,sum=%d\n",in,frame_id,e,f,e+f);
frame_id++;
ret=pthread_mutex_unlock(&mutex);
err(ret,"prod unlock error");
/* ③ 放一个产品：唤醒可能在等产品的消费者 */
ret=sem_post(&full);
if(ret==-1){
perror("prod full post error");
exit(1);
}
sleep(1);
}
printf("PROD finish\n");
pthread_exit(NULL);
}
void *cons(void *arg){
(void)arg;
int ret;
while(consumed<10){
/* ① 等一个产品：信号量无虚假唤醒，不需要 while */
ret=sem_wait(&full);
if(ret==-1){
perror("cons full wait error");
exit(1);
}
/* ② 临界区：取产品、推进下标 */
ret=pthread_mutex_lock(&mutex);
err(ret,"cons lock error");
int temp=buf[out];
out=(out+1)%N;
int e,f;
sem_getvalue(&empty,&e);
sem_getvalue(&full,&f);
printf("CONS:out=%d,frame_id=%d,empty=%d,full=%d,sum=%d,consumed=%d\n",out,temp,e,f,e+f,consumed);
consumed++;
ret=pthread_mutex_unlock(&mutex);
err(ret,"cons unlock error");
/* ③ 腾一个空槽：唤醒可能在等空槽的生产者 */
ret=sem_post(&empty);
if(ret==-1){
perror("cons empty post error");
exit(1);
}
sleep(1);
}
printf("CONS finish\n");
pthread_exit(NULL);
}
int main(){
pthread_t pid,cid;
int ret;
ret=sem_init(&empty,0,N);
if(ret==-1){
perror("main empty init error");
exit(1);
}
ret=sem_init(&full,0,0);
if(ret==-1){
perror("main full init error");
exit(1);
}
ret=pthread_create(&pid,NULL,prod,NULL);
err(ret,"main prod create error");
ret=pthread_create(&cid,NULL,cons,NULL);
err(ret,"main cons create error");
ret=pthread_join(pid,NULL);
err(ret,"main prod join error");
ret=pthread_join(cid,NULL);
err(ret,"main cons join error");
ret=pthread_mutex_destroy(&mutex);
err(ret,"main mutex destroy error");
ret=sem_destroy(&empty);
if(ret==-1){
perror("main empty destroy error");
exit(1);
}
ret=sem_destroy(&full);
if(ret==-1){
perror("main full destroy error");
exit(1);
}
pthread_exit(NULL);
}
