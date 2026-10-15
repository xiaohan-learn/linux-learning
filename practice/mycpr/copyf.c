#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc,char **argv){
    if(argc!=3){
        fprintf(stderr,"Usage:%s <srcfile> <dstfile>\n",argv[0]);
        return 1;
    }
    ssize_t wn,rn;

    int src_fd=open(argv[1],O_RDONLY);
    if(src_fd<0){
        perror("src open");
        return 1;
    }

    int dst_fd=open(argv[2],O_RDWR | O_CREAT |O_TRUNC,0644);
    if(dst_fd<0){
        perror ("dst open");
        close(src_fd);
        return 1;
    }

    char buf [1024];
    while((rn=read(src_fd,buf,sizeof(buf)))>0){
        wn=write(dst_fd,buf,rn);
        if(wn!=rn){
            perror("write");
            close(src_fd);
            close(dst_fd);
            return 1;
        }
    }

    if(rn<0){
        perror("read");
        close(src_fd);
        close(dst_fd);
        return 1;
    }

    printf("copy success\n");
    close(src_fd);
    close(dst_fd);
    return 0;

}