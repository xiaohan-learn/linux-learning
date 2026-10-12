#include <stdio.h>

int main(int argc,char **argv){
    if(argc!=3){
        fprintf(stderr,"Usage:%s <srcfile> <dstfile>",argv[0]);
        return 1;
    }

    FILE *src=fopen(argv[1],"rb");
    if(src==NULL){
        perror("src fopen");
        return 1;
    }

    FILE *dst=fopen(argv[2],"wb");
    if(dst==NULL){
        perror("dst fopen");
        fclose(src);
        return 1;
    }

    char buf[1024];
    size_t rn,wn;
    while((rn=fread(buf,1,1024,src))>0){
        wn=fwrite(buf,1,rn,dst);
        if(wn!=rn){
            perror("fwrite");
            fclose(src);
            fclose(dst);
            return 1;
        }
    }

    if((ferror(src))){
        perror("fread");
        fclose(src);
        fclose(dst);
        return 1;
    }else if(feof(src)){
        printf("读到文章末尾\n");
    }

    fclose(src);
    fclose(dst);
    return 0;

}