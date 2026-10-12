#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "student.h"

int new_score(const char *path,int line,int newscore){
    int fd=open(path,O_RDWR | O_CREAT,0644);
    if(fd<0){
        perror("open");
        return 1;
    }

    off_t offset=line*MAX_LEN;
    off_t n=lseek(fd,offset,SEEK_SET);
    if(n==(off_t)-1){
        perror("lseek");
        close(fd);
        return 1;
    }

    char buf[MAX_LEN];
    ssize_t rn=read(fd,buf,MAX_LEN);
    if(rn<0){
        perror("read");
        close(fd);
        return 1;
    }
    if(rn==0){
        fprintf(stderr,"行号超出文件范围\n");
        close(fd);
        return 1;
    }

    char score[SCORE_LEN+1];
    snprintf(score,sizeof(score),"%4d",newscore);
    memcpy(buf+ID_LEN+NAME_LEN,score,SCORE_LEN);

    off_t n1=lseek(fd,offset,SEEK_SET);
    if(n1==(off_t)-1){
        perror("lseek");
        close(fd);
        return 1;
    }

    ssize_t wn=write(fd,buf,MAX_LEN);
    if(wn!=MAX_LEN){
        perror("write");
        close(fd);
        return 1;
    }

    close (fd);
    return 0;

}