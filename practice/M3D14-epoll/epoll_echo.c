#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_EVENTS 1024

int main(){
    int listen_fd=socket(AF_INET,SOCK_STREAM,0);
    if(listen_fd==-1){
        perror("socket");
        return 1;
    }

    struct sockaddr_in lis_addr;
    lis_addr.sin_family=AF_INET;
    lis_addr.sin_port=htons(8888);
    lis_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    int opt=1;
    setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    
    int ret=bind(listen_fd,(struct sockaddr *)&lis_addr,sizeof(lis_addr));
    if(ret==-1){
        perror("bind");
        return 1;
    }

    ret=listen(listen_fd,5);
    if(ret==-1){
        perror("listen");
        return 1;
    }

    printf("服务器端口8888开始监听\n");

    int epfd=epoll_create1(0);
    if(epfd==-1){
        perror("epoll_create1");
        return 1;
    }

    struct epoll_event ev,events[MAX_EVENTS];
    ev.events=EPOLLIN;
    ev.data.fd=listen_fd;
    ret=epoll_ctl(epfd,EPOLL_CTL_ADD,listen_fd,&ev);
    if(ret==-1){
        perror("epoll_ctl");
        return 1;
    }

    while(1){
        int n=epoll_wait(epfd,events,MAX_EVENTS,-1);
        if(n==-1){
            perror("epoll_wait");
            return 1;
        }

        for(int i=0;i<n;i++){
            int fd=events[i].data.fd;
            if(fd==listen_fd){
                struct sockaddr_in cli_addr;
                socklen_t cli_len=sizeof(cli_addr);

                int newfd=accept(listen_fd,(struct sockaddr *)&cli_addr,&cli_len);
                if(newfd==-1){
                    perror("accept");
                    continue;
                }

                ev.events=EPOLLIN;
                ev.data.fd=newfd;
                ret=epoll_ctl(epfd,EPOLL_CTL_ADD,newfd,&ev);
                if(ret==-1){
                    perror("new epoll_ctl");
                    close(newfd);
                    continue;
                }
            }
            else{
                char buf[1024];
                memset(buf,0,sizeof(buf));
                ssize_t r=recv(fd,buf,sizeof(buf)-1,0);
                if(r<0){
                    perror("recv");
                    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
                    close(fd);
                    continue;
                }
                else if(r==0){
                    printf("客户端退出\n");
                    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
                    close(fd);
                    continue;
                }
                buf[r]='\0';
                printf("服务端收到消息:%s\n",buf);

                int total=0;
                while(total<r){
                    ssize_t s=send(fd,buf+total,r-total,0);
                    if(s<0){
                        perror("send");
                        return 1;
                    }
                    total+=s;
                }
            }
        }
    }
    close(epfd);
    close(listen_fd);
    return 0;
}