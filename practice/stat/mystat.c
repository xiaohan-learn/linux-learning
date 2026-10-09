#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
void mode_str(mode_t mode,char *s);

int main(int argc,char **argv){
    if(argc!=2){
        fprintf(stderr,"Usage:%s <file>\n",argv[0]);
        return 1;
    }

    struct stat st;
    if(stat(argv[1],&st)<0){
        perror("stat");
        return 1;
    }
    struct stat st1;
    if(lstat(argv[1],&st1)<0){
        perror("lstat");
        return 1;
    }

    // 1) 大小：st.st_size
    printf("stat文件大小=%ld\n",(long)st.st_size);
    printf("lstat文件大小=%ld\n",(long)st1.st_size);

    // 2) 权限：把 (st_mode & 0777) 转成 "rwxrwxrwx" 字符串（每 3 位一查）
    mode_t perm=st.st_mode&0777;
    printf("stat权限=%o\n",perm);
    mode_t perm1=st1.st_mode&0777;
    printf("lstat权限=%o\n",perm1);
    
    char str[10];
    mode_str(perm,str);
    printf("stat权限字符串=%s\n",str);
    char str1[10];
    mode_str(perm1,str1);
    printf("lstat权限字符串=%s\n",str1);

    // 3) 类型：S_ISREG/S_ISDIR/S_ISLNK 判断，打印 REG/DIR/LNK
    printf("=====stat判断文件类型=====\n");
    if(S_ISREG(st.st_mode)) printf("这是普通文件\n");
    else if(S_ISDIR(st.st_mode)) printf("这是目录\n");
    else if(S_ISLNK(st.st_mode)) printf("这是符号链接\n");
    else printf("其他文件\n");
    printf("=====lstat判断文件类型=====\n");
    if(S_ISREG(st1.st_mode)) printf("这是普通文件\n");
    else if(S_ISDIR(st1.st_mode)) printf("这是目录\n");
    else if(S_ISLNK(st1.st_mode)) printf("这是符号链接\n");
    else printf("其他文件\n");

    // 4) 硬链接数 st.st_nlink，inode st.st_ino
    printf("stat硬链接数=%lu\n",(unsigned long)st.st_nlink);
    printf("lstat硬链接数=%lu\n",(unsigned long)st1.st_nlink);
    printf("stat_inode=%lu\n",(unsigned long)st.st_ino);
    printf("latat_inode=%lu\n",(unsigned long)st1.st_ino);
    return 0;
}
void mode_str(mode_t mode,char *s){
    s[0]=(mode & S_IRUSR) ? 'r' : '-';
    s[1]=(mode & S_IWUSR) ? 'w' : '-';
    s[2]=(mode & S_IXUSR) ? 'x' : '-';
    s[3]=(mode & S_IRGRP) ? 'r' : '-';
    s[4]=(mode & S_IWGRP) ? 'w' : '-';
    s[5]=(mode & S_IXGRP) ? 'x' : '-';
    s[6]=(mode & S_IROTH) ? 'r' : '-';
    s[7]=(mode & S_IWOTH) ? 'w' : '-';
    s[8]=(mode & S_IXOTH) ? 'x' : '-';
    s[9]='\0';
}

