---
tags: [linux, m3, 系统编程, socket, 进程, 线程]
aliases: [M3, Linux系统编程, 系统编程]
created: 2026-09-30
---

# M3 · Linux 系统编程

> 时间：11/11–12/10 ｜ 主线：进程 / 线程 / IPC / socket / epoll
> 落地项目：[[#🎯 简历项目2 TCP 服务器]]
> 前置：[[M2-韦东山应用篇]] 的文件IO + gcc + Makefile 必须熟

## 📋 章节清单

### 1. 文件与 IO 进阶
- [ ] `open/read/write/lseek` 熟练（接 M2）
- [ ] `mmap` 内存映射文件（零拷贝思路）
- [ ] `fcntl` / `ioctl` 基础（M4 V4L2 要用 `ioctl`）
- [ ] 文件描述符复制 `dup/dup2`

### 2. 进程（process）
- [ ] `fork()` 写时复制、父子进程关系
- [ ] `wait/waitpid` 回收僵尸进程
- [ ] `exec` 族函数（替换进程映像）
- [ ] `signal` 信号：`SIGCHLD` / `SIGINT` / `SIGKILL` 不可捕获
- [ ] 守护进程 daemon 初步（选学）

### 3. 线程（pthread）
- [ ] `pthread_create` / `pthread_join` / `pthread_exit`
- [ ] 线程间共享地址空间 vs 进程隔离
- [ ] 互斥锁 `pthread_mutex_t`（防竞态）
- [ ] 条件变量 `pthread_cond_t`（选学）
- [ ] 线程安全概念

### 4. IPC（进程间通信）
- [ ] 匿名/命名管道 `pipe` / `mkfifo`
- [ ] 共享内存 `shmget/shmat`（最快 IPC）
- [ ] 消息队列（选学）
- [ ] 信号 `signal` 也归此处

### 5. 网络编程（socket）
- [ ] 网络字节序：`htonl/htons/ntohl/ntohs`（大端小端）
- [ ] TCP 流程：socket → bind → listen → accept → recv/send → close
- [ ] UDP 流程：socket → bind → recvfrom/sendto
- [ ] 阻塞 vs 非阻塞、`setsockopt`(SO_REUSEADDR)
- [ ] `select` / `poll` 多路复用初步

### 6. epoll（进阶，高并发核心）
- [ ] `epoll_create/epoll_ctl/epoll_wait`
- [ ] LT（水平触发）vs ET（边缘触发）
- [ ] 对比 `select`/`poll` 的 O(n) vs O(1)

## 🎯 简历项目2 TCP 服务器
- **目标**：每来一个客户端连接，开一个线程处理（pthread 每连接一线程）→ 进阶用 epoll 重构为单线程高并发
- **关键词**：Socket TCP / 并发 / 文件IO / 网络字节序
- **里程碑**：
  - [ ] 单连接 echo 服务器（能收能发）
  - [ ] 多线程版（每连接一线程，支持多客户端同时聊）
  - [ ] 线程池版（避免频繁创建线程）
  - [ ] epoll 版（ET + 非阻塞，单线程扛高并发）
- **易错点**：
  - 忘记 `SO_REUSEADDR` 导致端口占用 `bind` 失败
  - 网络字节序混用（端口/长度要 `htons/htonl`）
  - 读不全（TCP 是字节流，`recv` 一次不一定收满，要循环读）
  - 线程间共享数据结构没加锁
  - 客户端断开后服务端 `recv` 返回 0 要正确关闭

## 🔁 与 M2/M4 的衔接
- `ioctl` / `mmap` 是 [[M4-V4L2摄像头项目]] 的地基（V4L2 靠这俩操作摄像头缓冲）
- 多线程 + 文件IO 贯穿整个 V4L2 采集程序

## 🐞 我的坑位
- 

## 📚 参考
- 《UNIX 环境高级编程》（APUE）— 系统编程圣经，当字典
- 韦东山应用篇系统编程部分 + 尚硅谷嵌入式 Linux 应用层开发
- `man 2` / `man 7 socket` 查原型
