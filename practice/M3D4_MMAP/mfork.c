#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

int var=100;
int main(int argc,char **argv){
    if(argc!=2){
        fprintf(stderr,"Usage: %s <file>\n",argv[0]);
        return 1;
    }

    int fd=open(argv[1],O_RDWR | O_CREAT | O_TRUNC,0644);
    if(fd<0){
        perror("open");
        return 1;
    }

    ftruncate(fd,4);
    off_t len=lseek(fd,0,SEEK_END);
    if(len==(off_t)-1){
        perror("lseek");
        return 1;
    }

    int *p=mmap(NULL,len,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    //int *p=mmap(NULL,len,PROT_READ|PROT_WRITE,MAP_PRIVATE,fd,0);
    if(p==MAP_FAILED){
        perror("mmap");
        return 1;
    }

    pid_t pid=fork();
    if(pid<0){
        perror("fork");
        return 1;
    }
    else if(pid==0){
        *p=1000;
        var=200;
        printf("child,*p=%d,var=%d\n",*p,var);
    }
    else{
        sleep(1);
        printf("father,*p=%d,var=%d\n",*p,var);
        wait(NULL);
    }

    int ret=munmap(p,len);
    if(ret==-1){
        perror("munmap");
        return 1;
    }

    close(fd);
    return 0;
}