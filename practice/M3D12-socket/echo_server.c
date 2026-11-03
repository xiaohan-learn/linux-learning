#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

void err(int ret,char *str){
    if(ret==-1){
        perror(str);
        exit(1);
    }
}

int main(){
    /* ---------- 1. 创建监听套接字 ---------- */
    int ser_fd=socket(AF_INET,SOCK_STREAM,0);
    err(ser_fd,"socket");

    /* ---------- 2. 填充服务端地址 ---------- */
    struct sockaddr_in ser_addr;
    memset(&ser_addr,0,sizeof(ser_addr));
    ser_addr.sin_family=AF_INET;
    ser_addr.sin_port=htons(8888);
    ser_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    /* ---------- 3. 绑定 ---------- */
    int ret=bind(ser_fd,(struct sockaddr *)&ser_addr,sizeof(ser_addr));
    err(ret,"bind");

    /* ---------- 4. 监听 ---------- */
    ret=listen(ser_fd,5);
    err(ret,"listen");

    /* ---------- 5. 循环接受多个客户端 ---------- */
    while(1){
        struct sockaddr_in cli_addr;
        socklen_t cli_len=sizeof(cli_addr);

        int newfd=accept(ser_fd,(struct sockaddr *)&cli_addr,&cli_len);
        if(newfd==-1){
            perror("accept");
            continue;       // 单个客户端连接失败，不影响整体服务
        }

        /* ---------- 6. 与当前客户端通信 ---------- */
        char buf[1024];
        while(1){
            memset(buf,0,sizeof(buf));
            ssize_t n=recv(newfd,buf,sizeof(buf)-1,0);
            if(n<0){
                perror("recv");
                break;      // 该客户端出错，退出当前循环（不杀整个服务器）
            }
            if(n>0){
                buf[n]='\0';
                printf("服务端已收到信息:%s\n",buf);
            }else if(n==0){
                printf("客户端已退出\n");
                break;
            }

            /* 回显：只发送实际收到的字节数 n */
            ssize_t m=send(newfd,buf,n,0);
            if(m<0){
                perror("send");
                break;      // 单个客户端发送失败，退出当前循环
            }
            else if(m>0){
                printf("服务端已发送信息:%s\n",buf);
            }
        }
        /* ---------- 7. 关闭当前客户端连接 ---------- */
            close(newfd);
            printf("客户端连接已关闭，继续等待新连接...\n");
    }
    close(ser_fd);
    return 0;
}