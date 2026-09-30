---
tags: [linux, m5, h264, rtsp, 视频, 项目1]
aliases: [M5, H264, RTSP, 视频方向]
created: 2026-09-30
---

# M5 · H.264 硬编码 + RTSP 推流（简历项目1 升级）

> 时间：1/11–2/10 ｜ 主线：把 M4 的 jpg 采集升级成「H.264 硬编码 + RTSP 推流」
> **八股前置延续**：每周末继续简历 + 八股 + 模拟面试
> 目标：2/10 前确保两个核心项目（V4L2 + TCP）封箱可写进简历

## 📋 章节清单

### 1. 视频编码基础
- [ ] 为什么需要编码（原始 YUV 太大，H.264 压缩比高）
- [ ] 帧类型：I 帧 / P 帧 / B 帧
- [ ] 硬编码 vs 软编码（板上用硬件编码器，如树莓派 omx / RK 的 MPP）
- [ ] 码率 / 分辨率 / 帧率 概念

### 2. H.264 编码实现
- [ ] 硬件编码 API（取决于板子：RKMPP / omx / V4L2 m2m）
- [ ] 把 V4L2 采集的 YUV 喂给编码器 → 出 H.264 码流
- [ ] 封装成 Annex-B（start code 分隔 NALU）

### 3. RTSP 推流
- [ ] RTSP/RTP/RTCP 协议关系
- [ ] 用成熟库（live555 / FFmpeg）搭 RTSP 服务器
- [ ] 或用 FFmpeg 一条命令 `ffmpeg -f v4l2 ... -c:v h264 ... -f rtsp rtsp://...`
- [ ] VLC 客户端能拉流播放即成功

### 4. 备选备份轨：STM32 + FreeRTOS（仅追 MCU 岗才做）
- [ ] 江科大 STM32-HAL 教程（CubeMX / ADC / OLED / UART / 队列）
- [ ] FreeRTOS 任务/队列/信号量
- [ ] 传感器采集 demo（项目3 备选）

## 🎯 验收
- [ ] 板端摄像头画面能通过 RTSP 在电脑 VLC 实时观看
- [ ] 区分「硬编码」「软编码」「RTSP 协议」能讲清楚（面试高频）

## 🐞 我的坑位
- 

## 📚 参考
- FFmpeg 官方文档 / live555
- 板子厂商 H.264 编码示例（RKMPP / omx）
- 江科大 STM32-HAL、keysking FreeRTOS（备份轨）

## 🔗 衔接
- 采集地基见 [[M4-V4L2摄像头项目]]
- 网络地基见 [[M3-Linux系统编程]]
- 收尾见 [[M6-求职冲刺]]
