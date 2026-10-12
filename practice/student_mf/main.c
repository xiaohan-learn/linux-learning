#include <stdio.h>
#include "student.h"

int main(void){
    int line=0;
    int newscore;

    printf("请输入行数：\n");
    scanf("%d",&line);
    if(line<0){
        fprintf(stderr,"行数必须大于0\n");
        return 1;
    }

    printf("请输入新成绩：\n");
    scanf("%d",&newscore);
    if(newscore<0 || newscore>9999){
        fprintf(stderr,"修改成绩范围在0-9999之间\n");
        return 1;
    }

    int ret=new_score("student.txt",line,newscore);
    if(ret==0){
        printf("新成绩修改成功");
    }else{
        printf("成绩修改失败");
    }
    return 0;
}