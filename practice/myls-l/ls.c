#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

void mode_str(struct stat *m,char *s){
    switch (m->st_mode & S_IFMT)
    {
    case S_IFREG: s[0]='-'; break;
    case S_IFDIR: s[0]='d'; break;
    case S_IFLNK: s[0]='l'; break;
    default:s[0]='?';break;
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

int main(int argc,char **argv){
    if(argc!=2){
        fprintf(stderr,"Usage:%s <dir>\n",argv[0]);
        return 1;
    }

    DIR *dp=opendir(argv[1]);
    if(dp==NULL){
        perror("opendir");
        return 1;
    }
    struct dirent *ent;
    while((ent=readdir(dp))!=NULL){
        if(strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0){
            continue;
        }
        char path[1024];
        snprintf(path,sizeof(path),"%s/%s",argv[1],ent->d_name);

        struct stat st;
        if(lstat(path,&st)<0){
            perror("lstat");
            continue;
        }
        char str[11];
        mode_str(&st,str);

        printf("%s %lu %lld %s\n",str,
        (unsigned long)st.st_nlink,
        (long long)st.st_size,ent->d_name);
    }
    closedir(dp);
    return 0;

}