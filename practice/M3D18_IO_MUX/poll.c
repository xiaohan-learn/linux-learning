#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>
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

    struct pollfd fds[3];
    fds[0].fd=STDIN_FILENO;
    fds[0].events=POLLIN;
    fds[1].fd=lfd;
    fds[1].events=POLLIN;
    fds[2].fd=-1;
    fds[2].events=POLLIN;

    char buf[1024];
    while(1){
        memset(buf,0,sizeof(buf));
        ret=poll(fds,3,-1);
        if(ret<0){
            perror("poll");
            continue;
        }
        else if(ret==0){
            printf("poll timeout\n");
            continue;
        }

        // 标准输入就绪
        if(fds[0].revents & POLLIN){
            ssize_t r=read(fds[0].fd,buf,sizeof(buf)-1);
            if(r<0){
                perror("read");
                continue;
            }
            else if(r==0){
                printf("已读到文件末尾\n");
                continue;
            }
            buf[r]='\0';
            printf("stdin read:%s\n",buf);
        }

        // 监听fd就绪，有新连接
        if(fds[1].revents & POLLIN){
            struct sockaddr_in cli_addr;
            socklen_t cli_len=sizeof(cli_addr);
            int newfd=accept(lfd,(struct sockaddr *)&cli_addr,&cli_len);
            if(newfd==-1){
                perror("accept");
                continue;
            }
            if(fds[2].fd>0){
                printf("已有客户端，拒绝新连接\n");
                close(newfd);
                continue;
            }

            fds[2].fd = newfd;
            char ip[20];
            inet_ntop(AF_INET,&cli_addr.sin_addr,ip,sizeof(ip));
            printf("new client fd=%d,ip=%s,port=%d join\n",newfd,ip,ntohs(cli_addr.sin_port));
        }

        // 客户端fd就绪
        if(fds[2].fd>0 && fds[2].revents & POLLIN){
            ssize_t r2=read(fds[2].fd,buf,sizeof(buf)-1);
            if(r2<0){
                perror("read client");
                close(fds[2].fd);
                fds[2].fd = -1;
                continue;
            }
            else if(r2==0){
                printf("已读到文件末尾\n");
                close(fds[2].fd);
                fds[2].fd = -1;
                continue;
            }
            buf[r2]='\0';
            printf("client read:%s\n",buf);

            int total=0;
            while(total<r2){
                ssize_t w= write(fds[2].fd,buf+total,r2-total);
                if(w==-1){
                    perror("write");
                    close(fds[2].fd);
                    fds[2].fd = -1;
                    continue;
                }
                total+=w;
            }
        }
    }
    close(lfd);
    return 0;

}