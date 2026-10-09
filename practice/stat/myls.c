#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int main(int argc,char **argv){
    if(argc!=2){
        fprintf(stderr,"Usage:%s <dir path>\n",argv[0]);
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
      char path[512];
      snprintf(path,sizeof(path),"%s/%s",argv[1],ent->d_name);

      struct stat buf;
      if((lstat(path,&buf))<0){
        perror("lstat");
        continue;
      }

      if(S_ISREG(buf.st_mode)){
        printf("[F]%s\n",ent->d_name);
      }else if(S_ISDIR(buf.st_mode)){
        printf("[D]%s\n",ent->d_name);
      }else if(S_ISLNK(buf.st_mode)){
        printf("[L]%s\n",ent->d_name);
      }else
      printf("[O]%s\n",ent->d_name);
    }
    closedir(dp);
    return 0;
}