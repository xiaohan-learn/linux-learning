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

#define DEV_NODE "/dev/video0"  /*设备节点，上板换节点只改这里*/
#define WIDTH 640               /*请求宽*/
#define HEIGHT 480              /*请求高*/
#define JPEG_QUALITY 90         /*JPEG 压缩质量*/
#define REQ_BUF_COUNT 4         /*申请 4 块内核缓冲*/
#define PIXEL_FORMAT V4L2_PIX_FMT_YUYV
#define CAPTURE_FRAMES 5        /*抓 5 帧退出*/

int main(void){
    int fd=-1;                                  /*设备 fd，-1 表示未打开*/
    void *buffers[REQ_BUF_COUNT]={0};           /*存放每个内核缓冲区 mmap 后的用户态指针*/
    unsigned int buf_len[REQ_BUF_COUNT]={0};    /* 每块 mmap 的长度*/
    unsigned int actural_buf_cnt=0;             /*内核实际给的缓冲区数量*/
    int real_w=0,real_h=0;                      /*驱动S_FMT 读回的真实宽高*/
    uint8_t *rgb_buf=NULL;                      /*RGB 转换的临时缓冲区，预分配复用*/
    int ret=-1;                                 /*返回码，-1=失败*/
    int stream_on=0;                            /*成功开流置1*/

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

    /* ========== 8. 采集循环（核心） ========== */
    printf("[step 8] start capture loop,capture %d frames...\n ",CAPTURE_FRAMES);
    for(int i=0;i<CAPTURE_FRAMES;i++){
        struct v4l2_buffer buf;
        memset(&buf,0,sizeof(buf));
        buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory=V4L2_MEMORY_MMAP;
        /* DQBUF: 阻塞等待一帧数据就绪 */
        if((ioctl(fd,VIDIOC_DQBUF,&buf))<0){
            perror("VIDIOC_DQBUF");
            goto cleanup;
        }

        /* ===== 接口边界：采集输出 → 处理链输入 ===== */
        printf("frames %d:bytesused=%u\n",i,buf.bytesused);
        /* ====== 处理链 ====== */
        if(buf.bytesused<(unsigned)(real_h *real_w *2)){
            if((ioctl(fd,VIDIOC_QBUF,&buf))<0){     /*归还后重抓*/
                perror("VIDIOC_QBUF");
                goto cleanup;
            }    
            continue;
        }
        yuyv_to_rgb(buffers[buf.index],rgb_buf,real_w,real_h);

        char filename[32];
        snprintf(filename,sizeof(filename),"cap_%02d.jpg",i);
        if((write_jpeg(filename,rgb_buf,real_w,real_h,JPEG_QUALITY))==0){
            printf(" saved %s\n",filename);
        }

        /* 必须归还缓冲到内核，否则几帧后缓冲耗尽卡死 */
        if((ioctl(fd,VIDIOC_QBUF,&buf))<0){
            perror("VIDIOC_QBUF");
            goto cleanup;
        }
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
    return ret ? 1 :0;
}