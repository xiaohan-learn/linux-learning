#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void){
    int fd=open("test_fifo",O_RDONLY);
    if(fd<0){
        perror("open");
        exit(1);
    }
    printf("读端已打开,等待消息:\n");
    char buf[256];
    while(1){
        ssize_t rn=read(fd,buf,sizeof(buf));
        if(rn<0){
            perror("read");
            exit(1);
        }else if(rn==0){
            printf("读端已关闭\n");
            break;
        }
        buf[rn]='\0';
        printf("收到信息:%s\n",buf);
    }
    close(fd);
    return 0;
}