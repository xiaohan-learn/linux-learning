#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include "image_proc.h"  

/*1=本机假帧，0=板子真采集*/
#ifndef FAKE_CAPTURE
#define FAKE_CAPTURE 1          
#endif         
#define DEV_NODE "/dev/video0"  /*设备节点，上板换节点只改这里*/
#define WIDTH 640               /*请求宽*/
#define HEIGHT 480              /*请求高*/
#define JPEG_QUALITY 90         /*JPEG 压缩质量*/
#define REQ_BUF_COUNT 4         /*申请 4 块内核缓冲*/
#define PIXEL_FORMAT V4L2_PIX_FMT_YUYV
#define CAPTURE_FRAMES 5        /*抓 5 帧退出*/

#if FAKE_CAPTURE
static uint8_t *g_fake_yuyv=NULL;   /* 假帧缓冲，Step 4 初始化时 malloc,存放自己生成的 YUYV 图像*/
#endif

/*
 * 取一帧：成功返回 0，失败 -1。
 * 帧数据指针和真实字节数通过出参交给调用方——调用方不关心帧从哪来。
 *  - fd：V4L2 摄像头设备文件句柄 /dev/videoX，真采集才用，假模式没用
    - void *buffers[]：MMAP 映射出来的缓冲区数组，V4L2 把内核帧缓存映射到用户态，真采集用
    - struct v4l2_buffer *buf：V4L2 的 buffer 结构体，和驱动交互
    - uint8_t **frame：出参，输出拿到的图像数据起始地址（二重指针，函数内部修改外部指针的值）
    - unsigned int *used：出参，输出这一帧图像有效字节数
 */
static int acquire_frame(int fd,void *buffers[],struct v4l2_buffer *buf,int frame_no,uint8_t **frame,unsigned int *used){
 #if FAKE_CAPTURE
    (void)fd;(void)buffers;(void)buf;                   /*假模式用不到，消 -Wextra 警告*/
    gen_test_frame(g_fake_yuyv,WIDTH,HEIGHT,frame_no);    /*把图生成到全局缓冲 g_fake_yuyv*/
    *frame=g_fake_yuyv;                                 /*把图像缓冲区地址传给调用方*/
    *used=(unsigned int)(WIDTH * HEIGHT *2);
    return 0;

    /*#else进入真实 V4L2 采集分支*/
#else
    (void)frame_no;
    memset(buf,0,sizeof(*buf));
    buf->type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf->memory=V4L2_MEMORY_MMAP;
    if(ioctl(fd,VIDIOC_DQBUF,buf)<0){
        perror("VIDIOC_DQBUF");
        return -1;
    }

    *frame=(uint8_t *)buffers[buf->index];
    *used=(unsigned int)buf->bytesused;
    return 0;
#endif
}

/*
 * 归还帧：真模式 QBUF 把缓冲还给内核；假模式是自己的内存，空操作。
 - fd：摄像头句柄
 - buf：刚才 DQBUF 拿到的 buffer 结构体
 */
static void release_frame(int fd,struct v4l2_buffer *buf){
#if FAKE_CAPTURE
    (void)fd;(void)buf;     /*假模式：无需归还*/

    /*真采集分支*/
#else
    if(ioctl(fd,VIDIOC_QBUF,buf)<0){
        perror("VIDIOC_QBUF");
        return;
    }
#endif
}
int main(void){
    int fd=-1;                                  /*设备 fd，-1 表示未打开*/
    void *buffers[REQ_BUF_COUNT]={0};           /*存放每个内核缓冲区 mmap 后的用户态指针*/
    unsigned int buf_len[REQ_BUF_COUNT]={0};    /* 每块 mmap 的长度*/
    unsigned int actural_buf_cnt=0;             /*内核实际给的缓冲区数量*/
    int stream_on=0;                            /*成功开流置1*/ 

    int real_w=0,real_h=0;                      /*驱动S_FMT 读回的真实宽高*/
    uint8_t *rgb_buf=NULL;                      /*RGB 转换的临时缓冲区，预分配复用*/
    int ret=-1;                                 /*返回码，-1=失败*/ 

#if !FAKE_CAPTURE
    /* ========== 1. 打开视频设备 ========== */
    printf("[step 1] open %s...\n",DEV_NODE);
    fd=open(DEV_NODE,O_RDWR);
    if(fd<0){
        perror("open video device");
        goto cleanup;
    }

    /* ========== 2. 查询设备能力 ========== */
    printf("[step 2] VIDIOC_QUERYCAP...\n");
    struct v4l2_capability cap;
    memset(&cap,0,sizeof(cap));
    if((ioctl(fd,VIDIOC_QUERYCAP,&cap))<0){
        perror("VIDIOC_QUERYCAP");
        goto cleanup;
    }
    /*是否支持视频采集*/
    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
        fprintf(stderr,"error:not a video capture device\n");
        goto cleanup;
    }
    /*是否支持流式 IO*/
    if(!(cap.capabilities & V4L2_CAP_STREAMING)){
        fprintf(stderr,"device does not support streaming IO\n");
        goto cleanup;
    }

    /* ========== 3. 设置图像格式 ========== */
    printf("[step 3] VIDIOC_S_FMT...\n");
    struct v4l2_format fmt;
    memset(&fmt,0,sizeof(fmt));
    fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;   /*缓冲区类型，固定为视频采集*/
    fmt.fmt.pix.height=HEIGHT;
    fmt.fmt.pix.width=WIDTH;
    fmt.fmt.pix.pixelformat=PIXEL_FORMAT;
    fmt.fmt.pix.field=V4L2_FIELD_ANY;       /*场模式，V4L2_FIELD_ANY表示让驱动自动选择*/

    if((ioctl(fd,VIDIOC_S_FMT,&fmt))<0){
        perror("VIDIOC_S_FMT");
        goto cleanup;
    }
    /*必须读取驱动返回的真实值*/
    real_h=fmt.fmt.pix.height;
    real_w=fmt.fmt.pix.width;
    printf("real format : %d*%d, pixelformtat=0x%08x\n",
            real_w,real_h,fmt.fmt.pix.pixelformat);
    /*驱动不支持 YUYV*/
    if(fmt.fmt.pix.pixelformat !=PIXEL_FORMAT){
        fprintf(stderr,"error: device does not support YUYV format\n");
        goto cleanup;
    }

    /* ========== 4. 申请内核缓冲区 ========== */
    printf("[step 4] VIDIOC_REQBUFS...\n");
    struct v4l2_requestbuffers req;
    memset(&req,0,sizeof(req));
    req.count=REQ_BUF_COUNT;                /*请求 4 个缓冲区*/
    req.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory=V4L2_MEMORY_MMAP;            /*表示使用内存映射模式（零拷贝，性能最高）*/

    if((ioctl(fd,VIDIOC_REQBUFS,&req))<0){
        perror("VIDIOC_REQBUFS");
        goto cleanup;
    }
    /*至少需要 2 个缓冲区（双缓冲）才能正常流式工作*/
    if(req.count<2){
        fprintf(stderr,"error:insufficient buffer memory,got %d buffers\n",req.count);
        goto cleanup;
    }
    actural_buf_cnt=req.count;
    printf("got %u kernel buffers\n",actural_buf_cnt);

    /* ========== 5. 查询缓冲信息 + mmap 映射到用户态 ========== */
    printf("[step 5] VIDIOC_QUERYBUF +mmap...\n");
    for(unsigned int i=0;i<actural_buf_cnt;i++){
        struct v4l2_buffer buf;
        memset(&buf,0,sizeof(buf));
        buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory=V4L2_MEMORY_MMAP;
        buf.index=i;

        if((ioctl(fd,VIDIOC_QUERYBUF,&buf))<0){
            perror("VIDIOC_QUERYBUF");
            goto cleanup;
        }
        /*映射成功后，buffers[i] 就是用户态可以直接访问的缓冲区首地址。*/
        buffers[i]=mmap(NULL,buf.length,PROT_READ | PROT_WRITE,MAP_SHARED,fd,buf.m.offset);
        if(buffers[i]==MAP_FAILED){
            perror("mmap");
            goto cleanup;
        }

        buf_len[i]=buf.length;
        printf("buffer %u:offset=0x%08lx,length=%u\n",
                i,(unsigned long)buf.m.offset,buf.length);
    }

    /* ========== 6. 所有空缓冲入队，交给内核 ========== */
    printf("[step 6] VIDIOC_QBUF...\n");
    for(unsigned int i=0;i<actural_buf_cnt;i++){
        struct v4l2_buffer buf;
        memset(&buf,0,sizeof(buf));
        buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory=V4L2_MEMORY_MMAP;
        buf.index=i;

        if((ioctl(fd,VIDIOC_QBUF,&buf))<0){
            perror("VIDIOC_QBUF");
            goto cleanup;
        }
    }

    /* === 7. 开启数据流,摄像头开始真正采集数据，内核开始往队列里的缓冲区填数据。===*/
    printf("[step 7] VIDIOC_STREAMON...\n");
    /* 预分配 RGB 缓冲区只分配一次,循环复用，避免频繁 malloc/free */
    size_t rgb_size=(size_t)real_h * real_w *3;
    rgb_buf=(uint8_t *)malloc(rgb_size);
    if(!rgb_buf){
        perror("malloc rgb_buf");
        goto cleanup;
    }

    enum v4l2_buf_type type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if((ioctl(fd,VIDIOC_STREAMON,&type))<0){
        perror("VIDIOC_STREAMON");
        goto cleanup;
    }
    stream_on=1;

#else                       /*假模式的初始化*/
    real_w=WIDTH;
    real_h=HEIGHT;
    g_fake_yuyv=(uint8_t *)malloc((size_t)real_w *real_h *2);
    rgb_buf=(uint8_t *)malloc((size_t)real_w *real_h *3);
    if(!g_fake_yuyv || !rgb_buf){
        fprintf(stderr,"malloc fake buffers failed\n");
        goto cleanup;
    }
#endif

    /*====== 8.假模式采集循环换成接口调用======*/
    printf("[step 8] capture loop, %d frames...\n",CAPTURE_FRAMES);
    for(int i=0;i<CAPTURE_FRAMES;i++){
        uint8_t *frame=NULL;
        unsigned int used=0;
        struct v4l2_buffer buf;      /* 两种模式都需要，作为 acquire/release 的载体 */
        memset(&buf,0,sizeof(buf));

        if(acquire_frame(fd,buffers,&buf,i,&frame,&used)<0){
            goto cleanup;
        }

        printf("frame %d: used=%u\n", i, used);
        if (used < (unsigned)(real_w * real_h * 2)) {
            release_frame(fd, &buf);          /* 短帧归还，跳过 */
            continue;
        }

        yuyv_to_rgb(frame, rgb_buf, real_w, real_h);   /* 处理链：一行不改 */
        char name[32];
        snprintf(name, sizeof(name), "cap_%02d.jpg", i);
        if (write_jpeg(name, rgb_buf, real_w, real_h, JPEG_QUALITY) == 0)
            printf("  saved %s\n", name);

        release_frame(fd, &buf);
    }

    ret=0;
    printf("all frames captured successfully\n");

    /* ========== 9. 统一资源释放（成功/失败都会走到这里） ========== */
    cleanup:
    /* 先关流 */
    if(fd>=0 && stream_on){
        enum v4l2_buf_type ty=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(fd,VIDIOC_STREAMOFF,&ty);
    }

    /* 解除 mmap 映射 */
    for(unsigned int i=0;i<actural_buf_cnt;i++){
        if(buffers[i] && buffers[i] !=MAP_FAILED){
            munmap(buffers[i],buf_len[i]);
        }
    }
    free(rgb_buf);
    if (fd >= 0) {
        close(fd);
    }

#if FAKE_CAPTURE
    free(g_fake_yuyv);
#endif

    return ret ? 1 :0;
}