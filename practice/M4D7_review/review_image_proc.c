#include <stdio.h>
#include <stdint.h>
#include <jpeglib.h>
#include "review_image_proc.h"

static inline uint8_t clamp255(int v){
    if(v<0) return 0;
    if(v>255)   return 255;
    return (uint8_t)v;
}

void yuyv_to_rgb(const uint8_t *yuyv,uint8_t *rgb,int w,int h){
    const uint8_t *p=yuyv;
    uint8_t *q=rgb;
    int npix=w*h;

    for(int i=0;i<npix/2;i++){
        int y0=p[0];
        int  u=p[1]-128;
        int y1=p[2];
        int  v=p[3]-128;
        p+=4;

        int r_off=(359 *v) >>8;
        int g_off=(88 *u + 183 *v) >>8;
        int b_off=(454 *u) >>8;

        q[0]=clamp255(y0+r_off);
        q[1]=clamp255(y0-g_off);
        q[2]=clamp255(y0+b_off);
        q[3]=clamp255(y1+r_off);
        q[4]=clamp255(y1-g_off);
        q[5]=clamp255(y1+b_off);
        q+=6;
    }
}

int write_jpeg(const char *path,const uint8_t *rgb,int w,int h,int quality){
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *fp=fopen(path,"wb");
    if(!fp){
        perror("fopen");
        return 1;
    }

    cinfo.err=jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo,fp);

    cinfo.image_width=w;
    cinfo.image_height=h;
    cinfo.input_components=3;
    cinfo.in_color_space=JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo,quality,TRUE);
    jpeg_start_compress(&cinfo,TRUE);

    int stride=w*3;
    while(cinfo.image_height > cinfo.next_scanline){
        JSAMPROW row=(JSAMPROW)(rgb+stride *cinfo.next_scanline);
        jpeg_write_scanlines(&cinfo,&row,1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(fp);
    return 0;
}