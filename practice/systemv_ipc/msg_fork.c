#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/wait.h>

struct msg_buf{
    long mtype;
    char mtext[200];
};

int main(){
    key_t key=ftok(".",200);
    int msgid=msgget(key,IPC_CREAT | 0664);
    if(msgid==-1){
        perror("msgget");
        exit(1);
    }

    int ret;
    pid_t pid=fork();
    if(pid<0){
        perror("fork");
        exit(1);
    }
    else if(pid==0){
        struct msg_buf sbuf;
        sbuf.mtype=1;
        strcpy(sbuf.mtext,"hello msg fork");
        ret=msgsnd(msgid,&sbuf,strlen(sbuf.mtext)+1,0);
        if(ret==-1){
            perror("msgsnd");
            msgctl(msgid,IPC_RMID,NULL);
            exit(1);
        }
        printf("child send success\n");
    }
    else{
        sleep(1);
        struct msg_buf rbuf;
        ret=msgrcv(msgid,&rbuf,sizeof(rbuf.mtext),1,0);
        if(ret==-1){
            perror("msgrcv");
            msgctl(msgid,IPC_RMID,NULL);
            exit(1);
        }
        printf("parent recv: %s\n",rbuf.mtext);
        wait(NULL);
        msgctl(msgid,IPC_RMID,NULL);
    }
    return 0;
}