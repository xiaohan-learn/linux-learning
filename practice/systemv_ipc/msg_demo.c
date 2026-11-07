#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>

struct msg_buf{
    long mtype;
    char mtext[128];
};

int main(){
    key_t key=ftok(".",100);
    int msqid=msgget(key,IPC_CREAT | 0664);
    if(msqid==-1){
        perror("msgget");
        exit(1);
    }

    struct msg_buf buf;
    buf.mtype=1;
    strcpy(buf.mtext,"hello msg");

    int ret=msgsnd(msqid,&buf,strlen(buf.mtext)+1,0);
    if(ret==-1){
        perror("msgsnd");
        exit(1);
    }
    printf("send success\n");

    struct msg_buf rbuf;
    ret=msgrcv(msqid,&rbuf,sizeof(rbuf.mtext),1,0);
    if(ret==-1){
        perror("msgrcv");
        exit(1);
    }
    printf("recv : %s\n",rbuf.mtext);

    ret=msgctl(msqid,IPC_RMID,NULL);
    if(ret==-1){
        perror("msgctl");
        exit(1);
    }

    return 0;
}