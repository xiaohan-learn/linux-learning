#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#define N 5
int buf[N];
int in=0,out=0;
int frame_id=1;
int count=0;
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty=PTHREAD_COND_INITIALIZER;
pthread_cond_t not_full=PTHREAD_COND_INITIALIZER;
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
ret=pthread_mutex_lock(&mutex);
err(ret,"prod lock error");
while(count==N){
ret=pthread_cond_wait(&not_full,&mutex);
err(ret,"prod wait error");
}
buf[in]=frame_id;
in=(in+1)%N;
count++;
printf("PROD:in=%d,count=%d,frame_id=%d\n",in,count,frame_id);
frame_id++;
ret=pthread_cond_signal(&not_empty);
err(ret,"prod signal error");
ret=pthread_mutex_unlock(&mutex);
err(ret,"prod unlock error");
sleep(1);
}
printf("PROD finish\n");
pthread_exit(NULL);
}
void *cons(void *arg){
(void)arg;
int ret;
int consumed=0;
while(consumed<10){
ret=pthread_mutex_lock(&mutex);
err(ret,"cons lock error");
while(count==0){
ret=pthread_cond_wait(&not_empty,&mutex);
err(ret,"cons wait error");
}
int temp=buf[out];
out=(out+1)%N;
count--;
printf("CONS:out=%d,count=%d,temp=%d\n",out,count,temp);
consumed++;
ret=pthread_cond_signal(&not_full);
err(ret,"cons signal error");
ret=pthread_mutex_unlock(&mutex);
err(ret,"cons unlock error");
sleep(1);
}
printf("CONS finish\n");
pthread_exit(NULL);
}
int main(){
pthread_t pid,cid;
int ret;
ret=pthread_create(&pid,NULL,prod,NULL);
err(ret,"main prod create error");
ret=pthread_create(&cid,NULL,cons,NULL);
err(ret,"main cons create error");
ret=pthread_join(pid,NULL);
err(ret,"main prod join error");
ret=pthread_join(cid,NULL);
err(ret,"main cons join error");
ret=pthread_mutex_destroy(&mutex);
err(ret,"main mutex_destroy error");
ret=pthread_cond_destroy(&not_empty);
err(ret,"main not_empty destroy error");
ret=pthread_cond_destroy(&not_full);
err(ret,"main not_full destroy error");
pthread_exit(NULL);
}
