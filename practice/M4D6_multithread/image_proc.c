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

/* =====================================================================
 * 假帧源: 8 色彩条 (FULL-RANGE 值, 与 yuyv_to_rgb 的 FULL-RANGE 公式匹配)
 *        + 顶行一块 32px 白块随帧号右移, 肉眼可辨 cap_00..cap_04 不同
 * =================================================================== */
#if FAKE_CAPTURE
void gen_test_frame(uint8_t *yuyv, int w, int h, int frame_no){
    /* SMPTE 彩条近似值 (FULL-RANGE: Y 0~255) ,白、黄、青、绿、品红、红、蓝、黑*/
    static const uint8_t bars_y[8] = {235, 210, 170, 145, 106,  81,  41,  16};
    static const uint8_t bars_u[8] = {128,  16, 166,  54, 202,  90, 240, 128};
    static const uint8_t bars_v[8] = {128, 146,  16,  34, 222, 240, 118, 128};

    int bar_w = w / 8;          /*一根彩条的像素宽度*/
    if(bar_w < 2) bar_w = 2;    /*强制设为 2，防止bar_w=0导致除零或者画不出彩条*/

    /*tag_x0：方块左边界；tag_x1：方块右边界，方块宽度固定 32 像素*/
    int tag_x0 = (frame_no * 32) % w;
    int tag_x1 = tag_x0 + 32;

    uint8_t *p = yuyv;
    for(int y = 0; y < h; y++){             /*遍历每一行（y 是行号，从 0 到 h-1）*/
        for(int x = 0; x < w; x += 2){      /*横向遍历像素，一次步进 2 个像素*/
            int bar = x / bar_w;            /*根据当前像素 x 的位置，算出现在落在第几根彩条上（0~7）*/
            if(bar > 7) bar = 7;

            uint8_t Y = bars_y[bar], U = bars_u[bar], V = bars_v[bar];  /*先拿当前位置对应的彩条颜色*/

            if(y < 16 && x >= tag_x0 && x < tag_x1){   /* 帧号白块,如果落在标记区域，直接覆盖 YUV！*/
                Y = 235; U = 128; V = 128;             /* 用 235 而非 255, 与彩条同步 */
            }
            p[0] = Y; p[1] = U; p[2] = Y; p[3] = V;
            p += 4;
        }
    }
}
#endif