#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define ID_LEN 8
#define NAME_LEN 16
#define SCORE_LEN 4
#define MAX_LEN 28

int main(int argc,char **argv){
    int fd=open("student.txt",O_RDWR |O_CREAT |O_TRUNC,0644);
    if(fd<0){
        perror("open");
        return 1;
    }

    char stu[MAX_LEN+1];
    snprintf(stu,sizeof(stu),"%-8s%-16s%4d","20060001","zhangsan",70);
    write(fd,stu,MAX_LEN);

    snprintf(stu,sizeof(stu),"%-8s%-16s%4d","20060002","lisi",80);
    write(fd,stu,MAX_LEN);

    snprintf(stu,sizeof(stu),"%-8s%-16s%4d","20060003","wangwu",90);
    write(fd,stu,MAX_LEN);

    printf("student.txt已建立,内有三条学习记录\n");
    close(fd);
    return 0;

}