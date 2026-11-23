/* image_proc.h —— 图像处理模块的公共契约
 * 与帧源无关：softcam 的内存喂帧、camera 的 V4L2 采集都链接它。
 * 契约：函数返回 0 成功、非 0 失败；rgb 缓冲由调用方分配 (w*h*3)。
 *
 * 编译开关:
 *   FAKE_CAPTURE = 1  → 假帧源(本机 WSL 验证整条数据流)
 *   FAKE_CAPTURE = 0  → 真帧源(V4L2 八步 ioctl, 上板用)
 * 命令行可覆盖: gcc -DFAKE_CAPTURE=0 ...
 */
#ifndef IMAGE_PROC_H
#define IMAGE_PROC_H

#include <stdint.h>

#ifndef FAKE_CAPTURE
#define FAKE_CAPTURE 1
#endif

/* YUYV → RGB；rgb 缓冲由调用方分配，大小 w*h*3; */
void yuyv_to_rgb(const uint8_t *yuyv, uint8_t *rgb, int w, int h);

/* RGB 存 jpg；path 文件路径，quality 1~100；返回 0 成功，非 0 失败 */
int write_jpeg(const char *path, const uint8_t *rgb,
               int w, int h, int quality);

#if FAKE_CAPTURE
/* 假帧源: 生成一帧 w*h 的 YUYV 测试图, frame_no 控制顶行白块位置 */
void gen_test_frame(uint8_t *yuyv, int w, int h, int frame_no);
#endif

#endif