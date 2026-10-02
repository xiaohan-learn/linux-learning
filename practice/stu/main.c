#include <stdio.h>
#include "stu.h"

int main(){
	student s[]={
		{"zhang san",78},
		{"li si" ,89.5},
		{"wang wu",79}
	};
	int len=sizeof(s)/sizeof(s[0]);
	int max=find_top(s,len);
	printf("最高分的学生信息%s %.1f\n",s[max].name,s[max].score);
	return 0;
}

