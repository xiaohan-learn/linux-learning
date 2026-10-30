#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

void err(int ret,char *str){        //pthread error back
    if(ret!=0){
        fprintf(stderr,"%s : %s\n",str,strerror(ret));
        exit(1);
    }   
}

struct msg{
    int num;
    struct msg *next;
};
struct msg *head=NULL; 
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond=PTHREAD_COND_INITIALIZER;

void *producer(void *arg){
    int ret;
    (void)arg;
    while (1){
        struct msg *mp=malloc(sizeof(struct msg));
        if(mp==NULL){
            perror("malloc");
            exit(1);
        }

        ret=pthread_mutex_lock(&mutex);
        err(ret,"producer lock error");

        mp->num=rand()%1000;
        mp->next=head;
        head=mp;
        printf("producer num=%d\n",mp->num); fflush(stdout);

        ret=pthread_cond_signal(&cond);
        err(ret,"producer signal error");
        ret=pthread_mutex_unlock(&mutex);
        err(ret,"producer unlock error");
        sleep(1);
    }
    pthread_exit(NULL);
}

void *consumer(void *arg){
    int ret;
    long i=(long)arg;

    while(1){
        struct msg *mp;
        ret=pthread_mutex_lock(&mutex);
        err(ret,"consumer lock error");

        while(head==NULL){
            ret=pthread_cond_wait(&cond,&mutex);
            err(ret,"consumer wait error");
        }
        mp=head;
        head=mp->next;
        printf("consumer%ld,sum=%d\n",i,mp->num);

        ret=pthread_mutex_unlock(&mutex);
        err(ret,"consumer unlock error");
        sleep(1);
        free(mp);
    }
    pthread_exit(NULL);
}

int main(){
    pthread_t pid,cid[5];
    int i,ret;
    srand(time(NULL));

    ret=pthread_create(&pid,NULL,producer,NULL);
    err(ret,"producer create error");
    for(i=0;i<5;i++){
        ret=pthread_create(&cid[i],NULL,consumer,(void *)(long)i);
        err(ret,"consumer create error");
    }

    ret=pthread_join(pid,NULL);
    err(ret,"producer join error");
    for(i=0;i<5;i++){
        ret=pthread_join(cid[i],NULL);
        err(ret,"consumer join error");
    }

    ret=pthread_mutex_destroy(&mutex);
    err(ret,"mutex destroy error");
    ret=pthread_cond_destroy(&cond);
    err(ret,"cond deatroy error");

    pthread_exit(NULL);
}