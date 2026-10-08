#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define ID_LEN 8
#define NAME_LEN 16
#define SCORE_LEN 4
#define STU_MAX 28

int main(){
    int fd;
    ssize_t ret;
    fd=open("student.txt",O_RDWR | O_CREAT |O_TRUNC,0644);
    if(fd<0){
        perror("open");
        return 1;
    }

    char stu[STU_MAX+1];
    //第0行
    snprintf(stu,sizeof(stu),"%-8s%-16s%4d","20260001","ZhangSan",85);
    write(fd,stu,STU_MAX);
    //第一行
    snprintf(stu, sizeof(stu), "%-8s%-16s%4d", "20260002", "LiSi", 90);
    write(fd, stu, STU_MAX);
    // 第2行
    snprintf(stu, sizeof(stu), "%-8s%-16s%4d","20260003", "WangWu", 77);
    write(fd, stu, STU_MAX);

    close(fd);
    printf("student.txt 创建完成，共3条记录\n");
    return 0;
}
