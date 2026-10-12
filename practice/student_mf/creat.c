#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define ID_LEN 8
#define NAME_LEN 16
#define SCORE_LEN 4
#define MAX_LEN (ID_LEN+NAME_LEN+SCORE_LEN)

int main(int argc,char **argv){
    int fd=open("student.txt",O_RDWR | O_CREAT | O_TRUNC,0644);
    if(fd<0){
        perror("open");
        return 1;
    }

    char stu[MAX_LEN+1];
    snprintf(stu,sizeof(stu),"%-8s%-16s%4d","20260001","zhangsan",80);
    write(fd,stu,MAX_LEN);

    snprintf(stu,sizeof(stu),"%-8s%-16s%4d","20260002","lisi",90);
    write(fd,stu,MAX_LEN);

    snprintf(stu,sizeof(stu),"%-8s%-16s%4d","20260003","wangwu",70);
    write(fd,stu,MAX_LEN);

    close(fd);
    return 0;

}