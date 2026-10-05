#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc,char **argv){
    int src_fd,dst_fd;
    ssize_t rn;
    ssize_t wn;
    char buf[100];
    if(argc!=3){
        printf("Usage:%s <srcfile> <dstfile>\n",argv[0]);
        return -1;
    }
    
    //打开源文件，只读
    src_fd=open(argv[1],O_RDONLY);
    if(src_fd<0){
        perror("open src");
        close(src_fd);
        return -1;
    }
  
    //打开目标文件，只写
    dst_fd=open(argv[2],O_WRONLY | O_CREAT | O_TRUNC,0644);
    if(dst_fd<0){
        perror("open dst");
        close(dst_fd);
        return -1;
    }

    //循环读取复制
    while((rn=read(src_fd,buf,sizeof buf))>0){
        wn=write(dst_fd,buf,rn);
        if(wn!=rn){
            perror("write");
            close(dst_fd);
            close(src_fd);
            return -1;
        }
    }

    if(rn<0){
        perror("read");
        close(dst_fd);
        close(src_fd);
        return -1;
    }

    close(dst_fd);
    close(src_fd);
    return 0;
}