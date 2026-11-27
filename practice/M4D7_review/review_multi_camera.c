#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include "review_image_proc.h"

#define DEV_NODE "/dev/video0"
#define WIDTH 640
#define HEIGHT 480
#define JPEG_QUALITY 90
#define REQ_BUF_COUNT 4
#define CAPTURE_FRAMES 5
#define QUEUE_SIZE 4
#define PIXEL_FORMAT V4L2_PIX_FMT_YUYV

typedef struct{
    uint8_t *slot[QUEUE_SIZE];
    int size[QUEUE_SIZE];
    int in,out,count;
    int stop;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
}frame_queue_t;

static frame_queue_t que;
static int gfd=-1;
static void *buffers[REQ_BUF_COUNT]={0};
static unsigned int buf_len[REQ_BUF_COUNT]={0};
static unsigned int actural_buf_cnt=0;
static int real_w=0;
static int real_h=0;

static int acquire_frame(int fd,void *buffer[],struct v4l2_buffer *buf,uint8_t **frame,unsigned int *used){
    memset(buf,0,sizeof(*buf));
    buf->type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf->memory=V4L2_MEMORY_MMAP;
    if(ioctl(fd,VIDIOC_DQBUF,buf)<0){
        perror("VIDIOC_DQBUF");
        return -1;
    }

    *frame=(uint8_t *)buffer[buf->index];
    *used=(unsigned int)buf->bytesused;
    return 0;
}

static void release_frame(int fd,struct v4l2_buffer *buf){
    memset(buf,0,sizeof(*buf));
    buf->type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf->memory=V4L2_MEMORY_MMAP;
    if(ioctl(fd,VIDIOC_QBUF,buf)<0){
        perror("VIDIOC_QBUF");
        return;
    }
}

static void queue_init(void){
    size_t frame_sz=(size_t)(WIDTH *HEIGHT *2);
    for(int i=0;i<QUEUE_SIZE;i++){
        que.slot[i]=malloc(frame_sz);
        if(!que.slot[i]){
            perror("malloc");
            return;
        }
    }

    que.count=0;
    que.in=0;
    que.out=0;
    que.stop=0;
    pthread_mutex_init(&que.mutex,NULL);
    pthread_cond_init(&que.not_empty,NULL);
    pthread_cond_init(&que.not_full,NULL);
}

static void queue_destroy(void){
    for(int i=0;i<QUEUE_SIZE;i++){
        free(que.slot[i]);
    }

    pthread_mutex_destroy(&que.mutex);
    pthread_cond_destroy(&que.not_empty);
    pthread_cond_destroy(&que.not_full);
}

static void *producer(void *arg){
    (void)arg;
    int ret;
    for(int i=0;i<CAPTURE_FRAMES;i++){
        uint8_t *frame=NULL;
        unsigned int used=0;
        struct v4l2_buffer buf;
        memset(&buf,0,sizeof(buf));

        if(acquire_frame(gfd,buffers,&buf,&frame,&used)<0){
            break;
        }

        ret=pthread_mutex_lock(&que.mutex);
        if(ret!=0){
            fprintf(stderr,"producer lock error");      return NULL;
        }
        while(que.count >= QUEUE_SIZE && !que.stop){
            ret=pthread_cond_wait(&que.not_full,&que.mutex);
            if(ret!=0){
                fprintf(stderr,"producer wait error");      return NULL;
            }
        }
        if(que.stop){
            ret=pthread_mutex_unlock(&que.mutex);
            if(ret!=0){
                fprintf(stderr,"producer unlock error");      return NULL;
            }
            break;
        }
        memcpy(que.slot[que.in],frame,used);
        que.size[que.in]=used;
        que.in=(que.in+1)%QUEUE_SIZE;
        que.count++;

        ret=pthread_cond_signal(&que.not_empty);
        if(ret!=0){
            fprintf(stderr,"producer signal error");      return NULL;
        }
        ret=pthread_mutex_unlock(&que.mutex);
        if(ret!=0){
            fprintf(stderr,"producer unlock error");      return NULL;
        }

        release_frame(gfd,&buf);
    }

    ret=pthread_mutex_lock(&que.mutex);
    if(ret!=0){
        fprintf(stderr,"producer lock error");      return NULL;
    }
    que.stop=1;
    ret=pthread_cond_broadcast(&que.not_empty);
    if(ret!=0){
        fprintf(stderr,"producer empty broadcast error");      return NULL;
    }
    ret=pthread_cond_broadcast(&que.not_full);
    if(ret!=0){
        fprintf(stderr,"producer full broadcast error");      return NULL;
    }
    ret=pthread_mutex_unlock(&que.mutex);
    if(ret!=0){
        fprintf(stderr,"producer unlock error");      return NULL;
    }
    return NULL;
}
static void *consumer(void *arg){
    (void)arg;
    int ret;
    int n=0;
    size_t rgb_sz=(size_t)(WIDTH *HEIGHT *3);
    uint8_t *rgb=malloc(rgb_sz);
    if(!rgb){
        perror("rgb malloc");
        return NULL;
    }

    while(1){
        uint8_t *frame;
        ret=pthread_mutex_lock(&que.mutex);
        if(ret!=0){
            fprintf(stderr,"consumer lock error");      return NULL;
        }
        while(que.count ==0 && !que.stop){
            ret=pthread_cond_wait(&que.not_empty,&que.mutex);
            if(ret!=0){
                fprintf(stderr,"consumer wait error");      return NULL;
            }
        } 
        if(que.count==0 && que.stop){
            ret=pthread_mutex_unlock(&que.mutex);
            if(ret!=0){
                fprintf(stderr,"consumer unlock error");      return NULL;
            }
            break;
        }

        frame=que.slot[que.out];
        que.out=(que.out+1)%QUEUE_SIZE;
        que.count--;

        ret=pthread_cond_signal(&que.not_full);
        if(ret!=0){
            fprintf(stderr,"consumer signal error");      return NULL;
        }
        ret=pthread_mutex_unlock(&que.mutex);
        if(ret!=0){
            fprintf(stderr,"consumer unlock error");      return NULL;
        }
        
        yuyv_to_rgb(frame,rgb,WIDTH,HEIGHT);
        char filename[32];
        snprintf(filename,sizeof(filename),"th_%02d.jpg",n++);
        write_jpeg(filename,rgb,WIDTH,HEIGHT,JPEG_QUALITY);
        printf("saved %s\n",filename);
    }
    free(rgb);
    return NULL;
}

int main(void)
{
    int stream_on = 0;
    int ret = 1;
    int fd = -1;

    pthread_t pid, cid;

    /*1.打开视频设备*/
    printf("[step 1]open %s...\n",DEV_NODE);
    fd=open(DEV_NODE,O_RDWR);
    if(fd<0){
        perror("open");
        goto cleanup;
    }
    gfd = fd;

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

    // 初始化环形队列
    queue_init();
   
    /*7.开启数据流*/
    printf("[step 7] VIDIOC_STREAMON...\n");
    enum v4l2_buf_type ty=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(fd,VIDIOC_STREAMON,&ty)<0){
        perror("VIDIOC_STREAMON");
        goto cleanup;
    }
    stream_on=1;

    /*8.启动生产者消费者线程*/
    printf("[step 8] start capture loop,capture %d frames...\n",CAPTURE_FRAMES);
    ret=pthread_create(&pid,NULL,producer,NULL);
    if(ret!=0){
        fprintf(stderr,"producer craete error");
        return 1;
    }
    ret=pthread_create(&cid,NULL,consumer,NULL);
    if(ret!=0){
        fprintf(stderr,"consumer craete error");
        return 1;
    }

    ret=pthread_join(pid,NULL);
    if(ret!=0){
        fprintf(stderr,"producer join error");
        return 1;
    }
    ret=pthread_join(cid,NULL);
    if(ret!=0){
        fprintf(stderr,"consumer join error");
        return 1;
    }

    ret=0;
    printf("all frames captured successfully\n");

cleanup:
    queue_destroy();

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

    if(fd>=0){
        close(fd);
    }
    return ret ? 1 : 0;
}