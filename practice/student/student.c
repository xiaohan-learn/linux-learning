#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define ID_LEN 8
#define NAME_LEN 16
#define SCORE_LEN 4
#define MAX_LEN (ID_LEN+NAME_LEN+SCORE_LEN)

int main(int argc,char **argv){
    int fd;
    char buf[MAX_LEN];
    ssize_t rn,wn;
    int line=0;
    int newscore;

    fd=open("student.txt",O_RDWR |O_CREAT,0644);
    if(fd<0){
        perror("open");
        return 1;
    }

    printf("请输入要修改的行数：\n");
    scanf("%d",&line);
    if(line<0){
        fprintf(stderr,"行号输入无效\n");
        close(fd);
        return 1;
    }
    printf("请输入要修改的成绩：\n");
    scanf("%d",&newscore);
    if(newscore<0 || newscore>9999){
        fprintf(stderr,"成绩应在0-9999之间\n");
        close(fd);
        return 1;
    }


    off_t offset=line*MAX_LEN;
    off_t n=lseek(fd,offset,SEEK_SET);
    if(n==(off_t)-1){
        perror("lseek");
        close(fd);
        return 1;
    }

    rn=read(fd,buf,MAX_LEN);
    if(rn<0){
        perror("read");
        close(fd);
        return 1;
    }else if(rn==0){
        printf("已读到文章末尾\n");
        close(fd);
        return 1;
    }

    char score[SCORE_LEN+1];
    snprintf(score,sizeof(score),"%4d",newscore);
    memcpy(buf+ID_LEN+NAME_LEN,score,SCORE_LEN);

    lseek(fd,offset,SEEK_SET);
    wn=write(fd,buf,MAX_LEN);
    if(wn<0 || wn!=MAX_LEN){
        perror("write");
        close(fd);
        return 1;
    }


    printf("第%d行成绩已修改完成\n",line);
    close(fd);
    return 0;

}