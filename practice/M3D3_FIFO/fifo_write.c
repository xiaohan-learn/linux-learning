#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(void){
    int fifo=mkfifo("test_fifo",0644);
    if(fifo==-1 && errno != EEXIST){
        perror("mkfifo");
        exit(1);
    }
    int fd=open("test_fifo",O_WRONLY);
    if(fd<0){
        perror("open");
        exit(1);
    }
    printf("写端已连接,请输入信息:\n");
    char buf[256];
    while(1){
        if((fgets(buf,sizeof(buf),stdin))==NULL){
            break;
        }
        if(strncmp(buf,"quit",4)==0){
            break;
        }
        write(fd,buf,strlen(buf));
    }
    close(fd);
    printf("写端退出\n");
    return 0;
}