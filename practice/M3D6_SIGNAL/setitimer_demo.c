#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

int cnt=0;
void handler(int sig){
    if(sig==SIGALRM){
        cnt++;
        printf("第%d次触发SIGALRM\n",cnt);

        if(cnt>=5){
            printf("退出\n");
            exit(0);
        }
    }
}

int main(void){
    struct itimerval it;
    signal(SIGALRM,handler);

    it.it_interval.tv_sec=1;
    it.it_interval.tv_usec=0;
    it.it_value.tv_sec=1;
    it.it_value.tv_usec=0;

    int ret=setitimer(ITIMER_REAL,&it,NULL);
    if(ret==-1){
        perror("setitimer");
        exit(1);
    }

    while(1){
        pause();
    }
    return 0;
}