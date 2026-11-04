#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

void *worker(void *arg){
    int new_fd=*(int *)arg;
    free(arg);
    char buf[1024];
    while(1){
        memset(buf,0,sizeof(buf));
        ssize_t m=recv(new_fd,buf,sizeof(buf)-1,0);
        if(m<0){
            if(errno==EINTR)    {continue;}
            perror("recv");
            break;
        }
        else if(m==0){
            printf("客户端退出\n");
            break;
        }
        buf[m]='\0';
        printf("服务端收到信息:%s\n",buf);

        int total=0;
        while(total<m){
            ssize_t n=send(new_fd,buf+total,m-total,0);
            if(n<0){
                perror("send");
                break;
            }
            total+=n;
        }
    }
    close(new_fd);
    return(NULL);
}

int main(){
    int ser_fd=socket(AF_INET,SOCK_STREAM,0);
    if(ser_fd==-1){
        perror("socket");
        exit(1);
    }

    struct sockaddr_in ser_addr;
    memset(&ser_addr,0,sizeof(ser_addr));
    ser_addr.sin_family=AF_INET;
    ser_addr.sin_port=htons(8888);
    ser_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    int opt=1;
    setsockopt(ser_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    int ret=bind(ser_fd,(struct sockaddr *)&ser_addr,sizeof(ser_addr));
    if(ret==-1){
        perror("bind");
        exit(1);
    }

    ret=listen(ser_fd,5);
    if(ret==-1){
        perror("listen");
        exit(1);
    }
    printf("服务器启动，监听 8888 端口...\n");

    while(1){
        char ip[20];
        memset(ip,0,sizeof(ip));
        struct sockaddr_in cli_addr;
        socklen_t cli_len=sizeof(cli_addr);

        int new_fd=accept(ser_fd,(struct sockaddr *)&cli_addr,&cli_len);
        if(new_fd<0){
            perror("accept");
            break;
        }

        inet_ntop(AF_INET,&cli_addr.sin_addr,ip,sizeof(ip));
        printf("新连接ip:%s :port:%d\n",ip,ntohs(cli_addr.sin_port));

        int *fd=(int *)malloc(sizeof(int));
        if(fd==NULL){
            perror("malloc");
            return 1;
        }
        *fd=new_fd;

        pthread_t tid;
        int ret=pthread_create(&tid,NULL,worker,(void *)fd);
        if(ret!=0){
            fprintf(stderr,"pthread_create error:%s\n",strerror(ret));
            close(new_fd);
            free(fd);
            continue;
        }

        ret=pthread_detach(tid);
        if(ret!=0){
            fprintf(stderr,"pthread_detach error:%s\n",strerror(ret));
            close(new_fd);
            free(fd);
            continue;
        }

    }
    close(ser_fd);
    return 0;
}
