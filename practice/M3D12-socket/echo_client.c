#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

void err(int ret,char *str){
    if(ret==-1){
        perror(str);
        exit(1);
    }
}

int main(){
    /* ---------- 1. 创建客户端套接字 ---------- */
    int cli_fd=socket(AF_INET,SOCK_STREAM,0);
    err(cli_fd,"socket");

    /* ---------- 2. 填充目标服务端地址 ---------- */
    struct sockaddr_in ser_addr;     //这里是服务端地址
    memset(&ser_addr,0,sizeof(ser_addr));
    ser_addr.sin_family=AF_INET;
    ser_addr.sin_port=htons(8888);
    inet_pton(AF_INET,"127.0.0.1",&ser_addr.sin_addr);

    /* ---------- 3. 发起连接 ---------- */
    int ret=connect(cli_fd,(struct sockaddr *)&ser_addr,sizeof(ser_addr));
    err(ret,"connect");

    printf("已连接到服务端 127.0.0.1:8888,请输入要发送的内容(Ctrl+D 退出):\n");

    /* ---------- 4. 收发循环 ---------- */
    char buf[1024];
    while(1){
        memset(buf,0,sizeof(buf));

        /* 从键盘读取输入 */
        if(fgets(buf,sizeof(buf),stdin)==NULL){
            printf("输入结束,客户端退出\n");
            break;
        }
        
        /* 发送实际读到的长度（不含多余的 '\0'） */
        ssize_t m=send(cli_fd,buf,strlen(buf),0);
        if((int)m<0){
            perror("send");
            break;
        }
        if((int)m>0){
            printf("客户端已发送信息:%s\n",buf);
        }

        /* 接收服务端回显 */
        ssize_t n=recv(cli_fd,buf,sizeof(buf)-1,0);
        if (n < 0) {
            perror("recv");
            break;
        }
        if (n == 0) {
            printf("服务端已退出\n");
            break;   // 必须 break，否则继续 send 会触发 SIGPIPE 崩溃
        }
        buf[n] = '\0';
        printf("客户端已接收信息: %s", buf);
    }

    close(cli_fd);
    return 0;
}