#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc,char **argv){
    int n=10;
    if(argc==3){
        n=atoi(argv[2]);
        if(n<=0){
            fprintf(stderr,"打印行数必须大于0\n");
            return 1;
        }
    }

    if(argc!=2 && argc!=3){
        fprintf(stderr,"Usage:%s <file>\n %s <file> n\n",argv[0],argv[0]);
        return 1;
    }
    int fd=open(argv[1],O_RDONLY);
    if(fd<0){
        perror("open");
        return 1;
    }

    off_t off=lseek(fd,0,SEEK_END);
    if(off<0){
        perror("end lseek");
        close(fd);
        return 1;
    }
    if(off==0){
        fprintf(stderr,"空文件\n");
        close(fd);
        return 0;
    }

    char *buf=(char*)malloc(off+1);
    if(buf==NULL){
        perror("malloc");
        close (fd);
        return 1;
    }

    off_t size=lseek(fd,0,SEEK_SET);
    if(size<0){
        perror("set lseek");
        free(buf);
        close(fd);
        return 1;
    }

    off_t total=0;
    ssize_t rn;
    while(total<off){
        rn=read(fd,buf+total,off-total);
        if(rn<0){
            perror("read");
            free(buf);
            close(fd);
            return 1;
        }
        if(rn==0){
            fprintf(stderr,"已读到文章末尾\n");
            break;
        }
        total+=rn;
    }  
    buf[off]='\0';

    if (off > 0 && buf[off-1] == '\n')
    {n++;}          // 扫描前：末尾有换行则阈值 +1，抵消“空尾行”
    off_t start=0; // 默认打印全部（覆盖“不足 n 行”）
    int nl=0; // 已数到的换行符个数
    for(off_t i=off-1;i>=0;i--){
        if(buf[i]=='\n'){
            nl++;
           if(nl==n){
            start=i+1;
            break;
            }
        }
    }
    printf("%s\n",buf+start);
    free(buf);
    close(fd);
    return 0;
}