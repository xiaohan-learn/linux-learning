#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <string.h>

struct student{
    int id;
    int age;
    char name[50];
};
int main(int argc,char **argv){
    if(argc!=2){
        fprintf(stderr,"Usage: %s <file>\n",argv[0]);
        return 1;
    }

    struct student stu={1001,18,"zhangshan"};
    struct student *p;

    int fd=open(argv[1],O_RDWR | O_CREAT |O_TRUNC,0644);
    if(fd<0){
        perror("open");
        return 1;
    }

    ftruncate(fd,sizeof(stu));
    p=mmap(NULL,sizeof(stu),PROT_WRITE|PROT_READ,MAP_SHARED,fd,0);
    if(p==MAP_FAILED){
        perror("mmap");
        return 1;
    }

    memcpy(p,&stu,sizeof(stu));
    int ret=munmap(p,sizeof(stu));
    if(ret==-1){
        perror("munmap");
        return 1;
    }
    close (fd);
    return 0;

}