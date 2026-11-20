/* image_proc.h —— 图像处理模块的公共契约
 * 与帧源无关：softcam 的内存喂帧、camera 的 V4L2 采集都链接它。
 * 契约：函数返回 0 成功、非 0 失败；rgb 缓冲由调用方分配 (w*h*3)。
 */
#ifndef IMAGE_PROC_H
#define IMAGE_PROC_H

#include <stdint.h>   /* 函数签名用到 uint8_t */

/* YUYV → RGB；rgb 缓冲由调用方分配，大小 w*h*3; */
void yuyv_to_rgb(const uint8_t *yuyv, uint8_t *rgb, int w, int h);

/* RGB 存 jpg；path 文件路径，quality 1~100；返回 0 成功，非 0 失败 */
int write_jpeg(const char *path, const uint8_t *rgb,
               int w, int h, int quality);

#endif 