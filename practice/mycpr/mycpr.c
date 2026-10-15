#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

void mode_str(struct stat *m,char *s){
    switch(m->st_mode & S_IFMT){
        case S_IFREG : s[0]='-'; break;
        case S_IFDIR : s[0]='d'; break;
        case S_IFLNK : s[0]='l'; break;
        default : s[0]='?';
    }
    s[1]=(m->st_mode & S_IRUSR) ? 'r' : '-';
    s[2]=(m->st_mode & S_IWUSR) ? 'w' : '-';
    s[3]=(m->st_mode & S_IXUSR) ? 'x' : '-';
    s[4]=(m->st_mode & S_IRGRP) ? 'r' : '-';
    s[5]=(m->st_mode & S_IWGRP) ? 'w' : '-';
    s[6]=(m->st_mode & S_IXGRP) ? 'x' : '-';
    s[7]=(m->st_mode & S_IROTH) ? 'r' : '-';
    s[8]=(m->st_mode & S_IWOTH) ? 'w' : '-';
    s[9]=(m->st_mode & S_IXOTH) ? 'x' : '-';
    s[10]='\0';
}

int copy_file(const char *src,const char *dst){
    ssize_t wn,rn;
    int s=open(src,O_RDONLY);
    if(s<0){
        perror("s open");
        return 1;
    }

    int d=open(dst,O_WRONLY | O_CREAT |O_TRUNC,0644);
    if(d<0){
        perror("d open");
        close(s);
        return 1;
    }

    char buf[1024];
    while((rn=read(s,buf,sizeof(buf)))>0){
        wn=write(d,buf,rn);
        if(wn!=rn){
            perror("write");
            close(s);
            close(d);
            return 1;
        }
    }

    if(rn<0){
        perror("read");
        close(s);
        close(d);
        return 1;
    }

    close(s);
    close(d);
    return 0;
}

void mycpr(const char *src,const char *dst,int depth){
    struct stat st;
    if((lstat(src,&st))<0){
        perror("src lstat");
        return ;
    }

    if(S_ISDIR(st.st_mode)){
        if((mkdir(dst,st.st_mode & 0777))<0){
            perror("mkdir");
            return ;
        }

        DIR *dir=opendir(src);
        struct dirent *ent;
        while((ent=readdir(dir))!=NULL){
            if(strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0){
                continue;
            }

            char s_buf[4096];
            char d_buf[4096];
            snprintf(s_buf,sizeof(s_buf),"%s/%s",src,ent->d_name);
            snprintf(d_buf,sizeof(d_buf),"%s/%s",dst,ent->d_name);

            struct stat st1;
            if((lstat(s_buf,&st1))<0){
                perror("dst lstat");
                continue;
            }

            for(int i=0;i<depth;i++){printf("  ");}

            char str[11];
            mode_str(&st1,str);
            printf("%s %lld %s\n",str,(long long)st1.st_size,ent->d_name);

            if(S_ISDIR(st1.st_mode)){
                mycpr(s_buf,d_buf,depth+1);
            }else if(S_ISREG(st1.st_mode)){
                copy_file(s_buf,d_buf);
            }
        }
        closedir(dir);
    }
   
}

int main(int argc,char **argv){
    if(argc!=3){
        fprintf(stderr,"Usage:%s <src> <dst>\n",argv[0]);
        return 1;
    }
    mycpr(argv[1],argv[2],0);
    return 0;
}