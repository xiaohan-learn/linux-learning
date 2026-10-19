#include <stdio.h>
#include <unistd.h>

int var=100;
int main(){
	printf("before fork -1-\n");
	printf("before fork -2-\n");
	printf("before fork -3-\n");
	printf("before fork -4-\n");

	pid_t pid=fork();

	if(pid==-1){
		perror("fork error\n");
		return 1;
	}else if(pid==0){  //子进程fork返回值
		var=200;
		printf("child process var=%d,&var=%p\n",var,&var);
		printf("child is created,child id=%d,father id=%d\n",getpid(),getppid());
	}else if(pid>0){   //父进程fork返回值
		sleep(1);
		printf("father process var=%d,&var=%p\n",var,&var);
		printf("child id=%d,my id=%d,my father id=%d\n",pid,getpid(),getppid());
	}

	printf("========end of file======\n");

	return 0;
}

