#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/*1. 准备一个"表格文件" student.txt：每行一条记录、固定宽度（如 学号8 + 姓名16 + 成绩4）
2. open 文件（O_RDWR，既读又写）
3. 用户输入要改第几行 + 新成绩
4. lseek(fd, 行号 * 单行字节数, SEEK_SET)   // 跳到那条记录开头
5. read 出这一行到 buf，改成绩字段
6. lseek(fd, 行号 * 单行字节数, SEEK_SET)   // 写前再跳回（read 已把指针后移了！）
7. write 回去
8. close*/
#define ID_LEN 8
#define NAME_LEN 16
#define SCORE_LEN 4
#define STU_MAX 28

int main(){
    int fd;
    char buf[STU_MAX];
    int line_no;//(从第0行开始)
    int new_score;
    ssize_t ret;

    fd=open("student.txt",O_RDWR | O_CREAT,0644);
    if(fd<0){
        perror("open");
        return 1;
    }

    printf("请输入要修改的行数：\n");
    scanf("%d",&line_no);
    printf("请输入要修改的新成绩：\n");
    scanf("%d",&new_score);

    off_t offset=line_no*STU_MAX;
    off_t n=lseek(fd,offset,SEEK_SET);
    if(n==(off_t)-1){
        perror("lseek");
        close(fd);
        return 1;
    }

    ret=read(fd,buf,STU_MAX);
    if(ret<0){
        perror("read");
        close(fd);
        return 1;
    }else if(ret==0){
        printf("该行不存在，已读到文件末尾\n");
        close(fd);
        return 1;
    }

    if (new_score < 0 || new_score > 9999) {
    fprintf(stderr, "成绩必须在 0~9999\n");
    close(fd); return 1;
     }

    char score_buf[SCORE_LEN+1];
    snprintf(score_buf,sizeof(score_buf),"%4d",new_score);
    memcpy(buf+NAME_LEN+ID_LEN,score_buf,SCORE_LEN);


    lseek(fd,offset,SEEK_SET);

    ret=write(fd,buf,STU_MAX);
    if(ret<0){
        perror("write");
        close(fd);
        return 1;
    }

    if(ret!=STU_MAX){
        perror("write");
        close(fd);
        return 1;
    }
    printf("修改成功，第%d行已更新\n",line_no);

    close(fd);
    return 0;

}