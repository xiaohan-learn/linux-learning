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
 #include <jpeglib.h>

 #define WIDTH 640
 #define HEIGHT 480
 #define JPEG_QUAL 90

/* ---------------- 工具 ---------------- */
static inline uint8_t clamp255(int v){
    if(v<0)     return 0;
    if(v>255)   return 255;
    return (uint8_t)v;
}

/* ---------------- 1. 造帧: YUYV 测试图 ---------------- */
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

/* ---------------- 2. YUYV → RGB (整数运算) ---------------- */
/*
 * 浮点参考:
 *   R = Y + 1.402 * (V-128)
 *   G = Y - 0.344 * (U-128) - 0.714 * (V-128)
 *   B = Y + 1.772 * (U-128)
 *
 * 嵌入式定点化: 系数 × 256 (2^8), 结果 >>8。
 *   1.402*256 = 358.9 → 359
 *   0.344*256 =  88.1 →  88
 *   0.714*256 = 182.8 → 183
 *   1.772*256 = 453.6 → 454
 *
 * 为什么? 640×480 一帧 30 万像素, 每像素 3 次浮点乘加 → 近百万次,
 * 没 FPU 的 MCU 直接卡死; 有 FPU 也远不如整数乘法省电/省周期。
 * 精度损失 ≤ 1/256 ≈ 0.4 灰度级, 肉眼看不出。
 */
static void yuyv_to_rgb(const uint8_t *yuyv,uint8_t *rgb,int w,int h){
    const uint8_t *p=yuyv;
    uint8_t *q=rgb;
    int npix=w*h;
    /* 每轮吃 4 字节 YUYV, 吐 6 字节 RGB */
    for(int i=0;i<npix/2;i++){      
        int y0=p[0];
        int u=(int)p[1]-128;
        int y1=p[2];
        int v=(int)p[3]-128;
        p+=4;       /*指针前进 4 字节，指向下一对像素*/

        int r_off=(359*v) >> 8;
        int g_off=(88*u+183*v) >>8;
        int b_off=(454*u) >>8;

        /* 像素 0 */
        q[0]=clamp255(y0+r_off);
        q[1]=clamp255(y0-g_off);
        q[2]=clamp255(y0+b_off);
        /* 像素 1 (Y 不同,U/V 同上) */
        q[3]=clamp255(y1+r_off);
        q[4]=clamp255(y1-g_off);
        q[5]=clamp255(y1+b_off);
        q+=6;       /*指针前进 6 字节（2 像素 × 3 通道）*/
    }
} 

/* ---------------- 3. libjpeg 编码 ---------------- */
static int write_jpeg(const char *path,const uint8_t *rgb,int w,int h,int quality){
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *fp=fopen(path,"wb");
    if(!fp){
        return -1;
    }

    cinfo.err=jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo,fp);

    cinfo.image_height=h;
    cinfo.image_width=w;
    cinfo.input_components=3;
    cinfo.in_color_space=JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo,quality,TRUE);
    jpeg_start_compress(&cinfo,TRUE);

    int stride=w*3;
    while(cinfo.next_scanline<cinfo.image_height){
        JSAMPROW row=(JSAMPROW)(rgb+cinfo.next_scanline*stride);
        jpeg_write_scanlines(&cinfo,&row,1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(fp);
    return 0;
}

/* ---------------- main ---------------- */
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