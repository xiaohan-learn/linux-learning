#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

void mode_str(struct stat *m,char *s){
    switch(m->st_mode & S_IFMT){
        case S_IFREG : s[0]='-'; break;
        case S_IFDIR : s[0]='d'; break;
        case S_IFLNK : s[0]='l'; break;
        default : s[0]='?';
    }
    if(m->st_mode & S_IRUSR) s[1]='r'; else s[1]='-';
    if(m->st_mode & S_IWUSR) s[2]='w'; else s[2]='-';
    if(m->st_mode & S_IXUSR) s[3]='x'; else s[3]='-';
    if(m->st_mode & S_IRGRP) s[4]='r'; else s[4]='-';
    if(m->st_mode & S_IWGRP) s[5]='w'; else s[5]='-';
    if(m->st_mode & S_IXGRP) s[6]='x'; else s[6]='-';
    if(m->st_mode & S_IROTH) s[7]='r'; else s[7]='-';
    if(m->st_mode & S_IWOTH) s[8]='w'; else s[8]='-';
    if(m->st_mode & S_IXOTH) s[9]='x'; else s[9]='-';
    s[10]='\0';
}

int main(int argc ,char ** argv){
    if(argc!=2){
        fprintf(stderr,"Usage:%s <dir>\n",argv[0]);
        return 1;
    }

    DIR *dir=opendir(argv[1]);
    if(dir==NULL){
        perror("opendir");
        return 1;
    }

    struct dirent *ent;
    while((ent=readdir(dir))!=NULL){
        if(strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0){
            continue;
        }

        char path[1024];
        snprintf(path,sizeof(path),"%s/%s",argv[1],ent->d_name);

        struct stat st;
        if((lstat(path,&st))<0){
            perror("lstat");
            continue;
        }

        char str[11];
        mode_str(&st,str);

        printf("  %s %lld %s\n",str,(long long)st.st_size,ent->d_name);
    }

    closedir(dir);
    return 0;
}