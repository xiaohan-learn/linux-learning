/*
 * M4D1_softcam - 内存喂帧的 M4 存 jpg 里程碑
 *
 * 采集/处理解耦:
 *   gen_test_frame() 扮演"喂帧方"(今天内存,YUYV V4L2 格式)
 *   yuyv_to_rgb()    扮演"处理方",只吃 const uint8_t* yuyv
 *   write_jpeg()     扮演"落地方",只吃 const uint8_t* rgb
 * 换 帧源 = 换 gen_test_frame 的下一行,下游不动。
 *
 * 编译: gcc -Wall -Wextra softcam.c -o softcam -ljpeg
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <stdint.h>
 #include "image_proc.h"

 #define WIDTH 640
 #define HEIGHT 480
 #define JPEG_QUAL 90

/* ---------------- 造帧: YUYV 测试图 ---------------- */
/*
 * YUYV(也叫 YUY2) 内存布局:
 *   [Y0][U ][Y1][V ] [Y2][U'][Y3][V'] ...
 *   每 4 字节出 2 个像素, U/V 由两个水平相邻像素共享。
 *   每行字节数 = WIDTH * 2
 *
 * 图案:
 *   水平: 4 段色条, 用不同的 (U,V) 让色相变化
 *   垂直: Y 从 0 → 255 渐变, 让亮度从黑到白平滑过渡
 */
static void gen_test_frame(uint8_t *yuyv,int w,int h){
    /* 4 段颜色 (U, V), 128 是中性 */
    static const uint8_t colors[4][2]={
        {90,240},   /* 偏红 */
        {54,34},    /* 偏绿 */
        {240,110},  /* 偏蓝 */
        {128,128}   /* 中性灰 */
    };
    
    for(int y=0;y<h;y++){
        /*归一化到 0..255,最后一行能得到 255（纯白），第一行得 0（纯黑）*/
        uint8_t yv=(uint8_t)(y*255/(h-1));
        int row_byte=y*w*2;     /*行字节起始偏移*/

        for(int x=0;x<w;x+=2){
            int seg=(x*4)/w;    /*当前 x 落在第几段色条（0~3）*/
            uint8_t u=colors[seg][0];   /*取第 seg 组颜色的 U 色度分量*/
            uint8_t v=colors[seg][1];   /*取第 seg 组颜色的 V 色度分量*/
            int off=row_byte+(x/2)*4;   /*行内的字节偏移*/

            yuyv[off + 0] = yv;   /* Y0 */
            yuyv[off + 1] = u;    /* U  (Y0,Y1 共享) */
            yuyv[off + 2] = yv;   /* Y1 测试图简化，Y0/Y1共用亮度 */
            yuyv[off + 3] = v;    /* V  (Y0,Y1 共享) */

        }
    }
}

int main(void){
    size_t yuyv_size=(size_t)WIDTH*HEIGHT*2;
    size_t rgb_size=(size_t)WIDTH*HEIGHT*3;

    uint8_t *yuyv=(uint8_t *)malloc(yuyv_size);
    uint8_t*rgb=(uint8_t *)malloc(rgb_size);

    if(!yuyv || !rgb){
        fprintf(stderr,"malloc failed\n");
        if(yuyv)    free(yuyv);
        if(rgb)     free(rgb);
        return 1;
    }

    gen_test_frame(yuyv,WIDTH,HEIGHT);
    yuyv_to_rgb(yuyv,rgb,WIDTH,HEIGHT);

    if((write_jpeg("out.jpg",rgb,WIDTH,HEIGHT,JPEG_QUAL))!=0){
        if(yuyv)    free(yuyv);
        if(rgb)     free(rgb);
        return 1;
    }
    printf("wrote out.jpg(%d*%d)\n",WIDTH,HEIGHT);

    free(yuyv);
    free(rgb);
    return 0;
}