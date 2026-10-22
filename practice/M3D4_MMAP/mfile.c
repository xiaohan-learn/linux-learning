#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc,char **argv){
    if(argc!=2){
        fprintf(stderr,"Usage: %s <file>\n",argv[0]);
        return 1;
    }
    char buf[]="hello,mmap";

    int fd=open(argv[1],O_RDWR | O_CREAT |O_TRUNC,0644);
    if(fd==-1){
        perror("open");
        return 1;
    }

    // lseek(fd,100,SEEK_END);
    // write(fd,"\0",1);
    ftruncate(fd,100);
    off_t len=lseek(fd,0,SEEK_END);
    printf("len=%d\n",(int)len);

    char* p=mmap(NULL,len,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    if(p==MAP_FAILED){
        perror("mmap");
        return 1;
    }

    strcpy(p,buf);
    //memcpy(p,buf,strlen(buf));
    printf("%s\n",p);

    int ret=munmap(p,len);
    if(ret==-1){
        perror("munmap");
        return 1;
    }
    close(fd);
    return 0;
}