#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(){
    int sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd==-1){
        perror("socket");
        exit(1);
    }

    struct sockaddr_in ser_addr;
    memset(&ser_addr,0,sizeof(ser_addr));
    ser_addr.sin_family=AF_INET;
    ser_addr.sin_port=htons(8888);
    ser_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    int opt=1;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    int ret=bind(sockfd,(struct sockaddr *)&ser_addr,sizeof(ser_addr));
    if(ret==-1){
        perror("bind");
        exit(1);
    }
    printf("udp server on port8888...\n");

    char buf[1024];
    struct sockaddr_in cli_addr;
    socklen_t cli_len;
    char ip[20];

    while(1){
        memset(buf,0,sizeof(buf));
        cli_len=sizeof(cli_addr);
        ssize_t r=recvfrom(sockfd,buf,sizeof(buf)-1,0,(struct sockaddr *)&cli_addr,&cli_len);
        if(r<0){
            perror("recvfrom");
            continue;
        }

        inet_ntop(AF_INET,&cli_addr.sin_addr,ip,sizeof(ip));
        int p=ntohs(cli_addr.sin_port);
        buf[r]='\0';
        printf("recvfrom :%s : %d ,msg:%s\n",ip,p,buf);

        ssize_t s=sendto(sockfd,buf,r,0,(struct sockaddr *)&cli_addr,cli_len);
        if(s<0){
            perror("sendto");
            continue;
        }
    }
    close(sockfd);
    return 0;
}