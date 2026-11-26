#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include "review_image_proc.h"

#define DEV_NODE "/dev/video0"
#define WIDTH 640
#define HEIGHT 480
#define JPEG_QUALITY 90
#define REQ_BUF_COUNT 4
#define CAPTURE_FRAMES 5
#define PIXEL_FORMAT V4L2_PIX_FMT_YUYV

int main(void){
int fd=-1;
void *buffers[REQ_BUF_COUNT]={0};
unsigned int buf_len[REQ_BUF_COUNT]={0};
unsigned int actural_buf_cnt=0;
uint8_t *rgb=NULL;
int stream_on=0;
int real_w=0,real_h=0;
int ret=1;

/*1.打开视频设备*/
printf("[step 1]open %s...\n",DEV_NODE);
fd=open(DEV_NODE,O_RDWR);
if(fd<0){
    perror("open");
    goto cleanup;
}
/*2.查询设备能力*/
printf("[step 2] VIDIOC_QUERYCAP...\n");
struct v4l2_capability cap;
memset(&cap,0,sizeof(cap));
if(ioctl(fd,VIDIOC_QUERYCAP,&cap)<0){
    perror("VIDIOC_QUERY");
    goto cleanup;
}
if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
    fprintf(stderr,"error:not a video capture device\n");
    goto cleanup;
}
if(!(cap.capabilities & V4L2_CAP_STREAMING)){
    fprintf(stderr,"error:device does not support streaming IO\n");
    goto cleanup;
}
/*3.设置图像格式*/
printf("[step 3] VIDIOC_S_FMT...\n");
struct v4l2_format fmt;
memset(&fmt,0,sizeof(fmt));
fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
fmt.fmt.pix.height=HEIGHT;
fmt.fmt.pix.width=WIDTH;
fmt.fmt.pix.pixelformat=PIXEL_FORMAT;
fmt.fmt.pix.field=V4L2_FIELD_ANY;
if(ioctl(fd,VIDIOC_S_FMT,&fmt)<0){
    perror("VIDIOC_S_FMT");
    goto cleanup;
}

real_h=fmt.fmt.pix.height;
real_w=fmt.fmt.pix.width;
printf("real format: %d*%d,pixelformat=0x%08x\n",real_w ,real_h,fmt.fmt.pix.pixelformat);
if(fmt.fmt.pix.pixelformat !=PIXEL_FORMAT){
    fprintf(stderr,"device does not support YUYV format\n");
    goto cleanup;
}
/*4.申请缓冲区*/
printf("[step 4] VIDIOC_REQBUFS...\n");
struct v4l2_requestbuffers req;
memset(&req,0,sizeof(req));
req.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
req.count=REQ_BUF_COUNT;
req.memory=V4L2_MEMORY_MMAP;
if(ioctl(fd,VIDIOC_REQBUFS,&req)<0){
    perror("VIDIOC_REQBUFS");
    goto cleanup;
}

if(req.count<2){
    fprintf(stderr,"error:insuffient buffer memory,got %d buffers\n",req.count);
    goto cleanup;
}
actural_buf_cnt=req.count;
printf("got %u kernel buffers\n",actural_buf_cnt);
/*5.查询缓冲区+mmap*/
printf("[step 5] VIDIOC_QUERYBUF...\n");
for(unsigned int i=0;i<actural_buf_cnt;i++){
    struct v4l2_buffer buf;
    memset(&buf,0,sizeof(buf));
    buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory=V4L2_MEMORY_MMAP;
    buf.index=i;
    if(ioctl(fd,VIDIOC_QUERYBUF,&buf)<0){
        perror("VIDIOC_QUERYBUF");
        goto cleanup;
    }

    buffers[i]=mmap(NULL,buf.length,PROT_READ|PROT_WRITE,MAP_SHARED,fd,buf.m.offset);
    if(buffers[i]==MAP_FAILED){
        perror("mmap");
        goto cleanup;
    }
    buf_len[i]=buf.length;
    printf("buffers %u:offset=0x%08lx,length=%u\n",i,(unsigned long)buf.m.offset,buf_len[i]);
}
/*6.所有空缓冲入队，交给内核*/
printf("[step 6] VIDIOC_QBUF...\n");
for(unsigned int i=0;i<actural_buf_cnt;i++){
    struct v4l2_buffer buf;
    memset(&buf,0,sizeof(buf));
    buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory=V4L2_MEMORY_MMAP;
    buf.index=i;
    if(ioctl(fd,VIDIOC_QBUF,&buf)<0){
        perror("VIDIOC_QBUF");
        goto cleanup;
    }
}
/*7.开启数据流*/
printf("[step 7] VIDIOC_STREAMON...\n");
size_t rgb_sz=(size_t)(real_h *real_w *3);
rgb=malloc(rgb_sz);
if(!rgb){
    perror("malloc");
    goto cleanup;
}

enum v4l2_buf_type ty=V4L2_BUF_TYPE_VIDEO_CAPTURE;
if(ioctl(fd,VIDIOC_STREAMON,&ty)<0){
    perror("VIDIOC_STREAMON");
    goto cleanup;
}
stream_on=1;
/*8.采集循环*/
printf("[step 8] start capture loop,capture %d frames...\n",CAPTURE_FRAMES);
for(unsigned int i=0;i<CAPTURE_FRAMES;i++){
    struct v4l2_buffer buf;
    memset(&buf,0,sizeof(buf));
    buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory=V4L2_MEMORY_MMAP;
    if(ioctl(fd,VIDIOC_DQBUF,&buf)<0){
        perror("VIDIOC_DQBUF");
        goto cleanup;
    }

    printf("frames %d:bytesused=%u\n",i,buf.bytesused);
    if(buf.bytesused<(size_t)(real_h *real_w *2)){
        if(ioctl(fd,VIDIOC_QBUF,&buf)<0){
            perror("VIDIOC_QBUF");
            goto cleanup;
        }
        continue;
    }

    yuyv_to_rgb(buffers[buf.index],rgb,real_w,real_h);
    char filename[32];
    snprintf(filename,sizeof(filename),"cap_%02d.jpg",i);
    if(write_jpeg(filename,rgb,real_w,real_h,JPEG_QUALITY)==0){
        printf("saved %s\n",filename);
    }

    if(ioctl(fd,VIDIOC_QBUF,&buf)<0){
        perror("VIDIOC_QBUF");
        goto cleanup;
    }
}
ret=0;
printf("all frames captured successfully\n");

cleanup:
    if(fd>=0 && stream_on){
        enum v4l2_buf_type ty=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(ioctl(fd,VIDIOC_STREAMOFF,&ty)<0){
            perror("VIDIOC_STREAMOFF");
        }
    }

    for(unsigned int i=0;i<actural_buf_cnt;i++){
        if(buffers[i] && buffers[i] !=MAP_FAILED){
            munmap(buffers[i],buf_len[i]);
        }
    }

    free(rgb);
    if(fd>=0){
        close(fd);
    }

    return ret ? 1 : 0;
}
