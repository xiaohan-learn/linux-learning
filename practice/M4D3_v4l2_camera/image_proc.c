/* image_proc.c —— 图像处理模块实现
 * 内容从 softcam/camera 原样搬运，仅：
 *   1) 去掉 static（跨文件可见）
 *   2) write_jpeg 失败返回值统一为1
 * clamp255 是内部辅助，保持 static inline 不暴露。
 */
#include "image_proc.h"
#include <stdio.h>
#include <stdint.h>
#include <jpeglib.h>

/* ---------------- 内部辅助 ---------------- */
static inline uint8_t clamp255(int v){
    if(v < 0)   return 0;
    if(v > 255) return 255;
    return (uint8_t)v;
}

/* ---------------- YUYV → RGB (整数运算) ----------------
 * 浮点参考: R = Y + 1.402(V-128), G = Y - 0.344(U-128) - 0.714(V-128),
 *           B = Y + 1.772(U-128)
 * 定点化: 系数 ×256 后 >>8 (359 / 88 / 183 / 454)。
 * 注意: 这是 FULL-RANGE 公式 (Y 0~255)，上真摄像头需换 BT.601
 *       limited-range (298/409/100/208/516, 先 -16/-128)。
 */
void yuyv_to_rgb(const uint8_t *yuyv, uint8_t *rgb, int w, int h){
    const uint8_t *p = yuyv;
    uint8_t       *q = rgb;
    int npix = w * h;

    for(int i = 0; i < npix / 2; i++){
        int y0 = p[0];
        int u  = p[1] - 128;
        int y1 = p[2];
        int v  = p[3] - 128;
        p += 4;

        int r_off = (359 * v) >> 8;
        int g_off = (88 * u + 183 * v) >> 8;
        int b_off = (454 * u) >> 8;

        q[0] = clamp255(y0 + r_off);
        q[1] = clamp255(y0 - g_off);
        q[2] = clamp255(y0 + b_off);
        q[3] = clamp255(y1 + r_off);
        q[4] = clamp255(y1 - g_off);
        q[5] = clamp255(y1 + b_off);
        q += 6;
    }
}

/* ---------------- libjpeg 编码 ---------------- */
int write_jpeg(const char *path, const uint8_t *rgb,
               int w, int h, int quality){
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr       jerr;
    FILE *fp = fopen(path, "wb");
    if(!fp){
        perror("fopen");
        return 1;                 /* 统一：非 0 失败 */
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);

    cinfo.image_height     = h;
    cinfo.image_width      = w;
    cinfo.input_components = 3;
    cinfo.in_color_space   = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    int stride = w * 3;
    while(cinfo.image_height > cinfo.next_scanline){
        JSAMPROW row = (JSAMPROW)(rgb + stride * cinfo.next_scanline);
        jpeg_write_scanlines(&cinfo, &row, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(fp);
    return 0;
}