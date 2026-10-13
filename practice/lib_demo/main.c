#include <stdio.h>
#include "student.h"

int main(void){
    int line,newscore;

    printf("请输入行号:\n");
    scanf("%d",&line);
    if(line<0){
        fprintf(stderr,"行号必须大于0\n");
        return 1;
    }

    printf("请输入要修改的成绩:\n");
    scanf("%d",&newscore);
    if(newscore<0 || newscore>9999){
        fprintf(stderr,"成绩修改范围位0-9999\n");
        return 1;
    }

    int ret=modify_score("student.txt",line,newscore);
    if(ret==0){
        printf("第%d行成绩已修改成功\n",line);
    }else{
        printf("成绩修改失败\n");
    }

    return 0;

}