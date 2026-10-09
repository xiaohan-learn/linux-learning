#include <sys/stat.h>
#include <stdio.h>
#include <sys/types.h>
 #include <sys/sysmacros.h>

int main(int argc,char **argv){
    if(argc!=2){
        printf("Usage:%s <file>\n",argv[0]);
        return 1;
    }
    struct stat buf_stat,buf_lstat;
    int ret1=stat(argv[1],&buf_stat);
    if(ret1==-1){
        perror("buf_stat");
        return 1;
    }

    int ret2=lstat(argv[1],&buf_lstat);
    if(ret2==-1){
        perror("buf_lstat");
        return 1;
    }

    if (S_ISREG(buf_lstat.st_mode))  printf("这是普通文件\n");
    else if (S_ISLNK(buf_lstat.st_mode))  printf("这是符号链接\n");
    else if (S_ISDIR(buf_lstat.st_mode))  printf("这是目录\n");
    else if (S_ISCHR(buf_lstat.st_mode))  printf("这是字符设备\n");
    else if (S_ISBLK(buf_lstat.st_mode))  printf("这是块设备\n");
    else                                  printf("其他类型\n");

    mode_t perm1=buf_stat.st_mode & 0777;
    mode_t perm2=buf_stat.st_mode & 07777;
    printf("stat0777掩码权限=%o\n",perm1);
    printf("stat07777掩码权限=%o\n",perm2);

    mode_t perm3=buf_lstat.st_mode & 0777;
    mode_t perm4=buf_lstat.st_mode & 07777;
    printf("lstat0777掩码权限=%o\n",perm3);
    printf("lstat07777掩码权限=%o\n",perm4);

    printf("stat文件大小=%lld\n",(long long)buf_stat.st_size);
    printf("lstat文件大小=%lld\n",(long long)buf_lstat.st_size);

    printf("stat硬链接数=%lu\n",(unsigned long)buf_stat.st_nlink);
    printf("lstat硬链接数=%lu\n",(unsigned long)buf_lstat.st_nlink);

    printf("stat-inode号=%lu\n",(unsigned long)buf_stat.st_ino);
    printf("lstat-inode号=%lu\n",(unsigned long)buf_lstat.st_ino);

    printf("stat主设备号=%u\n",major(buf_stat.st_dev));
    printf("lstat主设备号=%u\n",major(buf_lstat.st_dev));

    printf("stat次设备号=%u\n",minor(buf_stat.st_dev));
    printf("lstat次设备号=%u\n",minor(buf_lstat.st_dev));


    return 0;
}