#include <stdio.h>

#define BUF_SIZE 4096

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <srcfile> <dstfile>\n", argv[0]);
        return 1;
    }

    FILE *src = fopen(argv[1], "rb");
    if (src == NULL) { perror("open src"); return 1; }

    FILE *dst = fopen(argv[2], "wb");
    if (dst == NULL) { perror("open dst"); fclose(src); return 1; }

    char buf[BUF_SIZE];
    size_t rn;
    /* 核心：一次读多少，就原样写多少，循环到读完(EOF) */
    while ((rn = fread(buf, 1, BUF_SIZE, src)) > 0) {
        size_t wn = fwrite(buf, 1, rn, dst);
        if (wn != rn) { perror("fwrite"); fclose(src); fclose(dst); return 1; }
    }

    if (ferror(src)) { perror("fread"); fclose(src); fclose(dst); return 1; }

    fclose(src);
    fclose(dst);
    printf("copy done\n");
    return 0;
}
