#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
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
#define CAPTURE_FRAMES 5        /*抓 5 帧退出*/
#define QUEUE_SIZE 4            /*环形队列槽位数量 4*/

typedef struct{
    uint8_t *slot[QUEUE_SIZE];  /* 每槽一帧 YUYV 缓冲（预分配） */
    int size[QUEUE_SIZE];       /* 该槽实际写入字节数 */
    int in,out,count;           /* (生产者）写位/（消费者）读位/当前队列里有效帧数 */
    int stop;                   /*全局停止标志，0 运行，1停止*/
    pthread_mutex_t mutex;      /*保护队列所有成员（in/out/count/slot/size/stop）*/
    pthread_cond_t not_empty;   /* 队列有帧了 → 唤醒消费者 */
    pthread_cond_t not_full;    /* 队列有空位 → 唤醒生产者 */
}frame_queue_t;
static frame_queue_t g_q;

#if FAKE_CAPTURE
static uint8_t *g_fake_yuyv=NULL;   /* 假帧缓冲，初始化时 malloc,存放自己生成的 YUYV 图像*/
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

/*环形队列初始化*/
static void queue_init(void){
    size_t frame_sz=(size_t)(WIDTH * HEIGHT *2);
    for(int i=0;i<QUEUE_SIZE;i++){
        g_q.slot[i]=malloc(frame_sz);       /*预分配内存*/
        if(!g_q.slot[i]){
            perror("frame malloc");
            return ;
        }
    }

    g_q.count=0;
    g_q.in=0;
    g_q.out=0;
    g_q.stop=0;
    pthread_mutex_init(&g_q.mutex,NULL);
    pthread_cond_init(&g_q.not_empty,NULL);
    pthread_cond_init(&g_q.not_full,NULL);
}
/*队列销毁*/
static void queue_destroy(void){
    for(int i=0;i<QUEUE_SIZE;i++){
        free(g_q.slot[i]);
    }

    pthread_mutex_destroy(&g_q.mutex);
    pthread_cond_destroy(&g_q.not_empty);
    pthread_cond_destroy(&g_q.not_full);
}

/*生产者线程*/
static void *producer(void *arg){
    (void)arg;
    int ret;
    /*循环取 V4L2 一帧*/
    for(int i =0;i<CAPTURE_FRAMES;i++){
        uint8_t *frame=NULL;     /*接收V4L2 驱动缓冲区的起始地址,用完要 release_frame还给驱动*/
        unsigned int used=0;     /*当前这一帧有效数据字节长度*/
        struct v4l2_buffer buf;
        memset(&buf,0,sizeof(buf));

        if(acquire_frame(-1,NULL,&buf,i,&frame,&used)<0){
            break;
        }

        /*拿队列锁，队列满就 while wait (not_full)*/
        ret=pthread_mutex_lock(&g_q.mutex);
        if(ret!=0){
            fprintf(stderr,"producer lock error");  return NULL;
        }

        while(g_q.count >= QUEUE_SIZE && !g_q.stop){
            ret=pthread_cond_wait(&g_q.not_full,&g_q.mutex);
            if(ret!=0){
                fprintf(stderr,"producer wait error");  return NULL;
            }
        }
        /*收到停止标记就退出*/
        if(g_q.stop){
            ret=pthread_mutex_unlock(&g_q.mutex);
            if(ret!=0){
                fprintf(stderr,"producer unlock error");  return NULL;
            }
            break;
        }

        /*把图像 memcpy 拷贝进环形队列槽，移动 in 指针、计数 + 1*/
        memcpy(g_q.slot[g_q.in],frame,used);
        g_q.size[g_q.in]=used;
        g_q.in=(g_q.in + 1)%QUEUE_SIZE;
        g_q.count++;

        /*signal 唤醒消费者（not_empty），解锁*/
        ret=pthread_mutex_unlock(&g_q.mutex);
        if(ret!=0){
            fprintf(stderr,"producer unlock error");  return NULL;
        }
        ret=pthread_cond_signal(&g_q.not_empty);
        if(ret!=0){
            fprintf(stderr,"producer signal error");  return NULL;
        }
/*真模式下把 V4L2 buffer 还回驱动,假模式无操作*/
    release_frame(-1,&buf);     /*应是真正的 fd，-1 只在假模式成立。*/
    }

    /*采集完成，置stop，广播唤醒阻塞线程*/
    ret=pthread_mutex_lock(&g_q.mutex);
    if(ret!=0){
        fprintf(stderr,"producer lock error");  return NULL;
    }
    g_q.stop=1;
    ret=pthread_cond_broadcast(&g_q.not_empty);
    if(ret!=0){
        fprintf(stderr,"producer empty broadcast error");  return NULL;
    }
    ret=pthread_cond_broadcast(&g_q.not_full);
    if(ret!=0){
        fprintf(stderr,"producer full broadcast error");  return NULL;
    }
    ret=pthread_mutex_unlock(&g_q.mutex);
    if(ret!=0){
        fprintf(stderr,"producer unlock error");  return NULL;
    }
    return NULL;
}

/*消费者线程*/
static void *consumer(void *arg){
    (void)arg;
    int ret;
    int n=0;        /*文件序号计数器*/
    size_t rgb_sz=(size_t)(WIDTH * HEIGHT *3);
    uint8_t *rgb=malloc(rgb_sz);    /*提前分配一块 RGB 缓存*/
    if(!rgb){
        perror("rgb malloc");
        return NULL;
    }

    /*死循环，持续消费帧；退出依靠内部 break（stop 标记 + 队列为空）*/
    while(1){
        uint8_t *frame;
        /*拿队列锁，队列空就 while wait (not_empty)*/
        ret=pthread_mutex_lock(&g_q.mutex);
        if(ret!=0){
            fprintf(stderr,"consumer lock error");  return NULL;
        }

        while(g_q.count==0 && !g_q.stop){
            ret=pthread_cond_wait(&g_q.not_empty,&g_q.mutex);
            if(ret!=0){
                fprintf(stderr,"consumer wait error");  return NULL;
            }
        }
        if(g_q.count==0 && g_q.stop){
            ret=pthread_mutex_unlock(&g_q.mutex);
            if(ret!=0){
                fprintf(stderr,"consumer unlock error");  return NULL;
            }
            break;
        }

        /*拿到帧、移动读指针、计数减一*/
        frame=g_q.slot[g_q.out];
        g_q.out=(g_q.out+1)%QUEUE_SIZE;
        g_q.count--;

        /*唤醒生产者、解锁*/
        ret=pthread_cond_signal(&g_q.not_full);
        if(ret!=0){
            fprintf(stderr,"consumer signal error");  return NULL;
        }
        ret=pthread_mutex_unlock(&g_q.mutex);
        if(ret!=0){
            fprintf(stderr,"consumer unlock error");  return NULL;
        }

        /*转格式、编码、存图*/
        yuyv_to_rgb(frame,rgb,WIDTH,HEIGHT);
        char name[32];
        snprintf(name,sizeof(name),"th_%02d.jpg",n++);
        write_jpeg(name,rgb,WIDTH,HEIGHT,JPEG_QUALITY);
        printf("saved %s\n",name);
    }
    /*收到停止且队列空，释放 rgb 缓存，线程退出*/
    free(rgb);
    return NULL;
}

int main(){
    pthread_t pid,cid;

#if FAKE_CAPTURE
    g_fake_yuyv=malloc((size_t)WIDTH * HEIGHT *2);
    if(!g_fake_yuyv){
        perror("fake yuyv malloc");
        return 1;
    }
#endif
    queue_init();

    int ret=pthread_create(&pid,NULL,producer,NULL);
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

    queue_destroy();

#if FAKE_CAPTURE
    free(g_fake_yuyv);
#endif

    printf("finished\n");
    return 0;
}
