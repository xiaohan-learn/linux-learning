#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc,char *argv[]){
    if(argc!=3){
        fprintf(stderr,"Usage:%s <ip> msg\n",argv[0]);
        exit(1);
    }

    int sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd==-1){
        perror("socket");
        exit(1);
    }

    struct sockaddr_in ser_addr;
    memset(&ser_addr,0,sizeof(ser_addr));
    ser_addr.sin_family=AF_INET;
    ser_addr.sin_port=htons(8888);
    inet_pton(AF_INET,argv[1],&ser_addr.sin_addr);
    socklen_t ser_len=sizeof(ser_addr);

    const char *msg=argv[2];
    size_t msg_len=strlen(msg);

    ssize_t s=sendto(sockfd,msg,msg_len,0,(struct sockaddr *)&ser_addr,ser_len);
    if(s<0){
        perror("sendto");
        close(sockfd);
        exit(1);
    }

    char buf[1024];
    memset(buf,0,sizeof(buf));

    ssize_t r=recvfrom(sockfd,buf,sizeof(buf)-1,0,(struct sockaddr *)&ser_addr,&ser_len);
    if(r<0){
        perror("recvfrom");
        close(sockfd);
        exit(1);
    }
    buf[r]='\0';
    printf("recvfrom :%s\n",buf);

    close(sockfd);
    return 0;
}