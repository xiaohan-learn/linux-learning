#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define MAX_CLIENTS 64
int g_clients[MAX_CLIENTS];     // 在线 fd，-1 表示空位
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

int client_add(int fd){
    int ret;
    ret=pthread_mutex_lock(&mutex);
    if(ret!=0){
        fprintf(stderr,"add lock error:%s\n",strerror(ret));
        exit(1);
    }

    int idx=-1;
    for(int i=0;i<MAX_CLIENTS;i++){
        if(g_clients[i]==-1){
            g_clients[i]=fd;
            idx=i;
            break;
        }
    }

    ret=pthread_mutex_unlock(&mutex);
    if(ret!=0){
        fprintf(stderr,"add unlock error:%s\n",strerror(ret));
        exit(1);
    }
    return idx;
}

void client_del(int fd){
    int ret;
    ret=pthread_mutex_lock(&mutex);
    if(ret!=0){
        fprintf(stderr,"del lock error:%s\n",strerror(ret));
        exit(1);
    }

    for(int i=0;i<MAX_CLIENTS;i++){
        if(g_clients[i]==fd){
            g_clients[i]=-1;
            break;
        }
    }

    ret=pthread_mutex_unlock(&mutex);
    if(ret!=0){
        fprintf(stderr,"del unlock error:%s\n",strerror(ret));
        exit(1);
    }
}

void broadcast(int sender_fd,const char *buf,int len){
    int ret;
    ret=pthread_mutex_lock(&mutex);
    if(ret!=0){
        fprintf(stderr,"broadcast lock error:%s\n",strerror(ret));
        exit(1);
    }

    for(int i=0;i<MAX_CLIENTS;i++){
        int c=g_clients[i];
        if(c==-1 || c==sender_fd){
            continue;       // 跳过空位、跳过发送者自己
        }

        int total=0;
        while(total<len){
            ssize_t n=send(c,buf+total,len-total,0);
            if(n<0){
                perror("send");
                break;
            }
            total+=n;
        }
    }

    ret=pthread_mutex_unlock(&mutex);
    if(ret!=0){
        fprintf(stderr,"broadcast unlock error:%s\n",strerror(ret));
        exit(1);
    }
}

void *worker(void *arg){
    int newfd=*(int *)arg;
    free(arg);

    int idx=client_add(newfd);     // ← 新：上线登记
    if(idx==-1){
        printf("连接已满，拒绝客户端 fd=%d\n", newfd);
        close(newfd);
        return NULL;
    }
    printf("新成员%d已加入聊天室\n",newfd);

    char buf[1024];
    while(1){
        memset(buf,0,sizeof(buf));
        ssize_t m=recv(newfd,buf,sizeof(buf)-1,0);
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
        broadcast(newfd,buf,m);     // ← 新：广播，而不是只回给本人

    }
    client_del(newfd);   // ← 新：下线注销   
    close(newfd);
    return NULL;
}

int main(){
    memset(g_clients,-1,sizeof(g_clients));

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
        struct sockaddr_in cli_addr;
        socklen_t cli_len=sizeof(cli_addr);
        int newfd=accept(ser_fd,(struct sockaddr *)&cli_addr,&cli_len);
        if(newfd<0){
            perror("accept");
            break;
        }

        inet_ntop(AF_INET,&cli_addr.sin_addr,ip,sizeof(ip));
        printf("新连接ip:%s : port:%d\n",ip,ntohs(cli_addr.sin_port));

        int *fd=(int *)malloc(sizeof(int));
        if(fd==NULL){
            perror("malloc");
            exit(1);
        }
        *fd=newfd;

        pthread_t tid;
        int ret=pthread_create(&tid,NULL,worker,(void *)fd);
        if(ret!=0){
            fprintf(stderr,"pthread_create error:%s\n",strerror(ret));
            close(newfd);
            free(fd);
            continue;
        }

        ret=pthread_detach(tid);
        if(ret!=0){
            fprintf(stderr,"pthread_detach error:%s\n",strerror(ret));
            close(newfd);
            free(fd);
            continue;
        }
    }
    close(ser_fd); 
    return 0;
}