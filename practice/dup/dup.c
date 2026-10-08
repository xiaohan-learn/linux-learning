#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

int main(int argc ,char **argv){
    int fd1 =open("1.txt",O_RDWR | O_CREAT |O_TRUNC,0644);
    if(fd1<0){
        perror("open");
        return 1;
    }
    int fd2=dup(fd1);
    printf("oldfd=%d\n",fd1);
    printf("newfd=%d\n",fd2);

    char str[]="hello world";
    write(fd1,str,strlen(str));
    lseek(fd1,0,SEEK_SET);

    char buf1[20];
    char buf2[20];
    ssize_t ret1=read(fd1,buf1,5);
    if(ret1<0){
        perror("read fd1");
        close(fd1);
        close(fd2);
        return 1;
    }
    buf1[ret1]='\0';
    ssize_t ret2=read(fd2,buf2,6);
    if(ret2<0){
        perror("read fd2");
        close(fd1);
        close(fd2);
        return 1;
    }
    buf2[ret2]='\0';
   
    printf("read from fd1:%s\n",buf1);
    printf("read from fd2:%s\n",buf2);

    close(fd1);
    close(fd2);
    return 0;
}