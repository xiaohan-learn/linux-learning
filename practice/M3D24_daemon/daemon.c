#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#define LOG_FILE "/tmp/daemon.log"
volatile sig_atomic_t running = 1;

// 信号处理：收到 SIGINT / SIGTERM 退出循环
void handler(int sig){      
    (void)sig;
    running=0;
}

int main(){
    pid_t pid1,pid2;

    // 1. 第一次 fork，父进程退出
    pid1=fork();
    if(pid1<0){
        perror("fork1");
        exit(1);
    }
    else if(pid1>0){
        exit(0);        // 父进程直接退出
    }

    // 子进程：2. 创建新会话，脱离终端（setsid）
    pid_t ret=setsid();
    if(ret==(pid_t)-1){
        perror("setsid");
        exit(1);
    }

    // 3. 第二次 fork（加固：防止子进程重新拿到控制终端）
    pid2=fork();
    if(pid2<0){
        perror("fork2");
        exit(1);
    }
    else if(pid2>0){
        exit(0);
    }

    // 4. 切换工作目录到根目录，防止占用挂载点
    chdir("/");
    // 5. 重置文件掩码
    umask(0);

    // 6. 关闭标准输入输出，重定向到 /dev/null
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    int fd=open("/dev/null",O_RDWR);
    if(fd==-1){
        perror("open");
        exit(1);
    }
    dup2(fd,STDIN_FILENO);
    dup2(fd,STDOUT_FILENO);
    dup2(fd,STDERR_FILENO);
    if(fd>2){
        close(fd);
    }

    // 注册信号
    struct sigaction sa;
    sa.sa_handler=handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=0;
    sigaction(SIGINT,&sa,NULL);
    sigaction(SIGTERM,&sa,NULL);

    // 主循环：每隔 2s 写日志
    FILE *fp=fopen(LOG_FILE,"a");
    if(fp==NULL){
        perror("fopen");
        exit(1);
    }

    while(running){
        time_t now=time(NULL);
        char *t=ctime(&now);
        t[strlen(t)-1] = '\0';      // 截掉ctime(&now)自带的 \n
        fprintf(fp,"[%s] daemon alive ,pid=%d\n",t,getpid());
        fflush(fp);
        sleep(2);
    }

    fclose(fp);
    return 0;
}