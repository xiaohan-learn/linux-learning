#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

void handler(int sig){
    (void)sig;
    printf("hello,sigprocmask\n");
}

void print(sigset_t *set){
    for(int i=1;i<32;i++){
        if(sigismember(set,i)){
            putchar('1');
        }else{
            putchar('0');
        }
    }
    printf("\n");
}

int main(void){
   struct sigaction set,oldset;
   sigset_t maskset ,retset,pendset;

   set.sa_handler= handler;            //注册回调函数
   sigemptyset(&set.sa_mask);          //置0，在回调函数工作期间
   set.sa_flags=0;                     //默认动作

   int ret=sigaction(SIGINT,&set,&oldset); 
   if(ret==-1){
    perror("sigaction");
    exit(1);
   }
   //不然pending集永远是全0
   sigemptyset(&maskset);
   sigaddset(&maskset,SIGINT);

   int n=sigprocmask(SIG_BLOCK,&maskset,&retset);  //屏蔽 SIGINT
   if(n==-1){
    perror("sigprocmask");
    exit(1);
   }

   kill(getpid(),SIGINT); //给自己发SIGINT
   //获取未决信号集并打印
   sigpending(&pendset);
   printf("pending信号集:");
   print(&pendset);

   sleep(2);
   printf("解除屏蔽\n");
   int n1=sigprocmask(SIG_UNBLOCK,&maskset,&retset);
   if(n1==-1){
    perror("sigprocmask");
    exit(1);
   }

   return 0;
}