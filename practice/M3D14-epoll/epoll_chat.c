#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <errno.h>

#define MAX_CLIENTS 64
#define MAX_EVENTS 1024
int g_clients[MAX_CLIENTS];

int add_fd(int fd){
    int idx=-1;
    for(int i=0;i<MAX_CLIENTS;i++){
        if(g_clients[i]==-1){
            g_clients[i]=fd;
            idx=i;
            break;
        }
    }
    return idx;
}

void del_fd(int fd){
    for(int i=0;i<MAX_CLIENTS;i++){
        if(g_clients[i]==fd){
            g_clients[i]=-1;
            break;
        }
    }
}

void broadcast(int sender_fd,const char *buf,int len){
    for(int i=0;i<MAX_CLIENTS;i++){
        if(g_clients[i]==-1 || g_clients[i]==sender_fd){
            continue;
        }

        int total=0;
        while(total<len){
            ssize_t s=send(g_clients[i],buf+total,len-total,0);
            if(s==-1){
                perror("send");
                break;
            }
            total+=s;
        }
    }
}

int main(){
    memset(g_clients,-1,sizeof(g_clients));
    int lis_fd=socket(AF_INET,SOCK_STREAM,0);
    if(lis_fd==-1){
        perror("socket");
        return 1;
    }

    struct sockaddr_in ser_addr;
    memset(&ser_addr,0,sizeof(ser_addr));
    ser_addr.sin_family=AF_INET;
    ser_addr.sin_port=htons(8888);
    ser_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    int opt=1;
    setsockopt(lis_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    int ret=bind(lis_fd,(struct sockaddr *)&ser_addr,sizeof(ser_addr));
    if(ret==-1){
        perror("bind");
        return 1;
    }

    ret=listen(lis_fd,5);
    if(ret==-1){
        perror("listen");
        return 1;
    }
    printf("服务端启动,监听8888端口...\n");

    int epfd=epoll_create1(0);
    if(epfd==-1){
        perror("epoll_create1");
        return 1;
    }

    struct epoll_event ev,events[MAX_EVENTS];
    ev.events=EPOLLIN;
    ev.data.fd=lis_fd;
    ret=epoll_ctl(epfd,EPOLL_CTL_ADD,lis_fd,&ev);
    if(ret==-1){
        perror("epoll_ctl");
        return 1;
    }

    while(1){
        int n=epoll_wait(epfd,events,MAX_EVENTS,-1);
        if(n<0){
            perror("epoll_wait");
            continue;
        }

        for(int i=0;i<n;i++){
            int fd=events[i].data.fd;
            if(fd==lis_fd){
                struct sockaddr_in cli_addr;
                socklen_t cli_len=sizeof(cli_addr);

                int newfd=accept(lis_fd,(struct sockaddr *)&cli_addr,&cli_len);
                if(newfd==-1){
                    perror("accept");
                    continue;
                }

                if(add_fd(newfd)==-1){
                    printf("连接已满,拒绝客户端fd=%d\n",newfd);
                    close(newfd);
                    continue;
                }

                ev.events=EPOLLIN;
                ev.data.fd=newfd;
                ret=epoll_ctl(epfd,EPOLL_CTL_ADD,newfd,&ev);
                if(ret==-1){
                    perror("new epoll_ctl");
                    del_fd(newfd);
                    close(newfd);
                    continue;
                }
                printf("新成员fd=%d已加入\n",newfd);
            }

            else{
                char buf[1024];
                memset(buf,0,sizeof(buf));
                ssize_t r=recv(fd,buf,sizeof(buf)-1,0);
                if(r<0){
                    if(errno==EINTR) {continue;}
                    perror("recv");
                    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
                    del_fd(fd);
                    close(fd);
                    continue;
                }
                else if(r==0){
                    printf("客户端fd=%d已退出\n",fd);
                    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
                    del_fd(fd);
                    close(fd);
                    continue;
                }
                buf[r]='\0';
                printf("服务端已收到信息:%s\n",buf);
                broadcast(fd,buf,r);
            }
        }
    }
    close(lis_fd);
    close(epfd);
    return 0;
}