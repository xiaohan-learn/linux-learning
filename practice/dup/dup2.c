#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc,char **argv){
    int fd=open("1.txt",O_RDWR | O_CREAT |O_TRUNC,0644);
    if(fd<0){
        perror("open");
        return 1;
    }

    int fd1=dup2(fd,1);
    printf("打印在文件");
    
    close(fd);
    return 0;

}