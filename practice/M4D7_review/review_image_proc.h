#ifndef REVIEW_IMAGE_PROC_H
#define REVIEW_IMAGE_PROC_H

#include <stdint.h>

void yuyv_to_rgb(const uint8_t *yuyv,uint8_t *rgb,int w,int h);

int write_jpeg(const char *path,const uint8_t *rgb,int w,int h,int quality);
#endif