#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

static long long files,dirs,links,total;

void mode_str(struct stat *m,char *s){
    switch(m->st_mode & S_IFMT){
        case S_IFREG : s[0]='-'; break;
        case S_IFDIR : s[0]='d'; break;
        case S_IFLNK : s[0]='l'; break;
        default : s[0]='?';
    }

    s[1]=(m->st_mode & S_IRUSR) ? 'r' :'-';
    s[2]=(m->st_mode & S_IWUSR) ? 'w' :'-';
    s[3]=(m->st_mode & S_IXUSR) ? 'x' :'-';
    s[4]=(m->st_mode & S_IRGRP) ? 'r' :'-';
    s[5]=(m->st_mode & S_IWGRP) ? 'w' :'-';
    s[6]=(m->st_mode & S_IXGRP) ? 'x' :'-';
    s[7]=(m->st_mode & S_IROTH) ? 'r' :'-';
    s[8]=(m->st_mode & S_IWOTH) ? 'w' :'-';
    s[9]=(m->st_mode & S_IXOTH) ? 'x' :'-';
    s[10]='\0';
}

void list_dir(const char *path,int depth){
    DIR *dir=opendir(path);
    if(dir==NULL){
        perror("opendir");
        return ;
    }

    struct dirent *ent;
    while((ent=readdir(dir))!=NULL){
        if(strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0){
        continue;
        }

        char full[1024];
        snprintf(full,sizeof(full),"%s/%s",path,ent->d_name);

        struct stat st;
        if((lstat(full,&st))<0){
        perror("lstat");
        continue;
        }

        for(int i=0;i<depth;i++){
            printf("  ");
        }
        char str[11];
        mode_str(&st,str);
        printf("%s %lld %s\n",str,(long long)st.st_size,ent->d_name);

        if(S_ISREG(st.st_mode)){
            files++;
        }else if(S_ISDIR(st.st_mode)){
            dirs++;
        }else if(S_ISLNK(st.st_mode)){
            links++;
        }
        total+=st.st_size;

        if(S_ISDIR(st.st_mode)){
            list_dir(full,depth+1);
        }
    }
    closedir(dir);
}

int main(int argc,char **argv){
    if(argc!=2){
        fprintf(stderr,"Usage:%s <dir>\n",argv[0]);
        return 1;
    }

    list_dir(argv[1],0);
    printf("=========汇总==========\n");
    printf("文件数=%lld,目录数=%lld,链接数=%lld,总字节数量=%lld\n",
    (long long)files,(long long)dirs,(long long)links,(long long)total);
    
    return 0;
}