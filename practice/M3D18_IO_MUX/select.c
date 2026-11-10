#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <signal.h>

int main(){
    signal(SIGPIPE, SIG_IGN);
    int lfd=socket(AF_INET,SOCK_STREAM,0);
    if(lfd==-1){
        perror("socket");
        exit(1);
    }

    struct sockaddr_in ser_addr;
    memset(&ser_addr,0,sizeof(ser_addr));
    ser_addr.sin_family=AF_INET;
    ser_addr.sin_port=htons(8888);
    ser_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    int opt=1;
    setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    int ret=bind(lfd,(struct sockaddr *)&ser_addr,sizeof(ser_addr));
    if(ret==-1){
        perror("bind");
        exit(1);
    }

    ret=listen(lfd,5);
    if(ret==-1){
        perror("listen");
        exit(1);
    }
    printf("server start on port:8888...\n");

    fd_set rfd;
    int connfd=-1;
    char buf[1024];
    while(1){
        memset(buf,0,sizeof(buf));
        FD_ZERO(&rfd);
        FD_SET(STDIN_FILENO,&rfd);
        FD_SET(lfd,&rfd);
        if(connfd>0)    FD_SET(connfd,&rfd);

        int maxfd=(connfd>lfd) ? connfd : lfd;
        if (maxfd < STDIN_FILENO)   maxfd = STDIN_FILENO;
        ret=select(maxfd+1,&rfd,NULL,NULL,NULL);
        if(ret<0){
            perror("select");
            continue;
        }
        else if(ret==0){
            printf("time out\n");
            continue;
        }

        // 键盘输入就绪
        if(FD_ISSET(STDIN_FILENO,&rfd)){
            ssize_t r=read(STDIN_FILENO,buf,sizeof(buf)-1);
            if(r<0){
                perror("read");
                continue;
            }
            else if(r==0){
                printf("已读到文件末尾\n");
                continue;
            }
            buf[r]='\0';
            printf("stdin read :%s\n",buf);
        }

        // 有新客户端连接
        if(FD_ISSET(lfd,&rfd)){
            struct sockaddr_in cli_addr;
            socklen_t cli_len=sizeof(cli_addr);
            int newfd=accept(lfd,(struct sockaddr *)&cli_addr,&cli_len);
            if(newfd==-1){
                perror("accept");
                continue;
            }

            char ip[20];
            inet_ntop(AF_INET,&cli_addr.sin_addr,ip,sizeof(ip));
            printf("new client fd=%d,ip=%s,port=%d join\n",newfd,ip,ntohs(cli_addr.sin_port));
            if(connfd > 0)
            {
                printf("已有客户端，拒绝新连接\n");
                close(newfd);
            }
            else
            {
                connfd=newfd;
            }
        }

        // 客户端socket有数据
        if(connfd>0 && FD_ISSET(connfd,&rfd)){
            ssize_t r2=read(connfd,buf,sizeof(buf)-1);
            if(r2<0){
                perror("read client");
                close(connfd);
                connfd=-1;
                continue;
            }
            else if(r2==0){
                printf("已读到文件末尾\n");
                close(connfd);
                connfd=-1;
                continue;
            }
            buf[r2]='\0';
            printf("connfd read :%s\n",buf);

            ssize_t total = 0;
            while (total < r2) {
                ssize_t w= write(connfd,buf+total,r2-total);
                if(w==-1){
                    perror("write");
                    close(connfd);
                    connfd = -1;
                    continue;
                }
                total+=w;
            }
        }
    } 
    close (lfd);
    return 0;  
}