#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include<arpa/inet.h>
#include <errno.h>
#include <signal.h>

#define MAX_EVENTS 1024

int main(){
    signal(SIGPIPE,SIG_IGN);
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
    printf("server on port8888...\n");

    int epfd=epoll_create1(0);
    if(epfd==-1){
        perror("epoll_create1");
        exit(1);
    }

    struct epoll_event ev,events[MAX_EVENTS];
    ev.data.fd=lfd;
    ev.events=EPOLLIN;
    ret=epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&ev);
    if(ret==-1){
        perror("add lfd epoll_ctl");
        exit(1);
    }

    while(1){
        int n=epoll_wait(epfd,events,MAX_EVENTS,-1);
        if(n==-1){
            perror("epoll_wait");
            continue;
        }

        for(int i=0;i<n;i++){
            int fd=events[i].data.fd;
            if(fd==lfd){
                struct sockaddr_in cli_addr;
                socklen_t cli_len=sizeof(cli_addr);
                int newfd=accept(lfd,(struct sockaddr *)&cli_addr,&cli_len);
                if(newfd==-1){
                    perror("accept");
                    continue;
                }

                ev.data.fd=newfd;
                ev.events=EPOLLIN;
                ret=epoll_ctl(epfd,EPOLL_CTL_ADD,newfd,&ev);
                if(ret==-1){
                    perror("add newfd epoll_ctl");
                    close(newfd);
                    continue;
                }

                char ip[20];
                inet_ntop(AF_INET,&cli_addr.sin_addr,ip,sizeof(ip));
                printf("new client fd=%d,ip=%s,port=%d join \n",newfd,ip,ntohs(cli_addr.sin_port));
            }
            else{
                char buf[1024];
                memset(buf,0,sizeof(buf));
                ssize_t r=recv(fd,buf,sizeof(buf)-1,0);
                if(r<0){
                    if(errno==EINTR)    continue;
                    perror("recv");
                    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
                    close(fd);
                    continue;
                }
                else if(r==0){
                    printf("client %d exitd\n",fd);
                    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
                    close(fd);
                    continue;
                }
                buf[r]='\0';
                printf("server recv:%s\n",buf);

                int total=0;
                while(total<r){
                    ssize_t s=send(fd,buf+total,r-total,0);
                    if(s<0){
                        perror("send");
                        epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
                        close(fd);
                        break;
                    }
                    total+=s;
                }
            }
        }
    }
    close(lfd);
    return 0;
}