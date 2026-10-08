#include<stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

int main(int argc,char **argv){
    int src_fd,dst_fd;
    ssize_t rn,wn;
    char buf[100];


    if(argc!=3){
        printf("Usage:%s <srcfile> <dstfile>\n",argv[0]);
        return 1;
    }

    src_fd=open(argv[1],O_RDONLY);
    if(src_fd<0){
        perror("src open");
        return 1;
    }

    dst_fd=open(argv[2],O_WRONLY |O_CREAT | O_TRUNC,0644);
    if(dst_fd<0){
        perror("dst open");
        close(src_fd);
        return 1;
    }

    
    while((rn=read(src_fd,buf,sizeof buf))>0){
        wn=write(dst_fd,buf,rn);
        if(wn!=rn){
            perror("dst write");
            close(dst_fd);
            close(src_fd);
            return 1;
        }
    }

     if(rn<0){
        perror("src read");
        close(src_fd);
        close(dst_fd);
        return 1;
    }

    close(src_fd);
    close(dst_fd);
    return 0;


}