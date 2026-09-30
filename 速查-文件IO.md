---
tags: [linux, 速查, 文件IO, 系统调用]
aliases: [文件IO速查, 系统调用IO]
created: 2026-09-30
---

# 速查 · 文件 IO

> 配合 [[M2-韦东山应用篇#5 文件 IO]] 使用。
> 两层：① **标准库** `fopen` 系列（C 已学，缓冲，见 `ch19`）；② **系统调用** `open` 系列（无缓冲，直接进内核，M2 新学）。

## 标准库 IO（C 层，已掌握）
```c
#include <stdio.h>
FILE *fp = fopen("a.txt", "r");   // r/w/a/rb/wb/ab
fprintf(fp, "hi %d\n", 1);
fgets(buf, n, fp);                 // 读一行(含\n)
fgetc(fp); fputc(c, fp);
fwrite(&s, sizeof(s), 1, fp);      // 二进制整块写
fread(&s, sizeof(s), 1, fp);
fclose(fp);                        // 必须配对！
```

## 系统调用 IO（应用层重点，man 2）
```c
#include <fcntl.h>    // open
#include <unistd.h>   // read/write/close/lseek
#include <sys/stat.h>

int fd = open("a.txt", O_RDONLY);            // 返回文件描述符, -1 失败
int fd2 = open("b.txt", O_WRONLY|O_CREAT, 0644); // 创建要给权限
char buf[1024];
ssize_t n = read(fd, buf, sizeof(buf));      // 返回实际读到的字节, 0=EOF, -1=错
write(fd2, buf, n);
lseek(fd, 0, SEEK_SET);                      // 移动读写位置
close(fd); close(fd2);
```

## 核心概念：文件描述符 fd
- fd 是**非负整数**，是进程打开文件的"句柄"
- 0=标准输入(stdin) / 1=标准输出(stdout) / 2=标准错误(stderr)
- `open` 返回最小可用 fd（通常从 3 开始）

## fopen vs open 区别（面试常问）
| | fopen | open |
|---|---|---|
| 层级 | 标准库(C库) | 系统调用(内核) |
| 缓冲 | 有用户态缓冲 | 无（直接系统调用）|
| 返回值 | `FILE*` | `int fd` |
| 跨平台 | 好 | Unix/Linux 专有 |
| 用法 | 文本/二进制都方便 | 更适合底层/设备文件 |

> [!note] 实际项目里：普通文本文件用 `fopen` 简单；操作设备(`/dev/video0`)、管道、需要 `fcntl/ioctl` 时用 `open`。

## 实现一个 cp 命令（M2 验收练习）
```c
// cp src dst
int in = open(argv[1], O_RDONLY);
int out = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0644);
char buf[4096];
ssize_t n;
while ((n = read(in, buf, sizeof(buf))) > 0)
    write(out, buf, n);
close(in); close(out);
```

## 易错点（对照 C 仓库 BUG.md）
- `open` 失败返回 `-1`，必须判错再继续
- `read` 一次不一定读满 `sizeof(buf)`（尤其设备或网络）→ 循环读
- 忘记 `close` → 文件描述符泄漏
- 创建文件忘给权限 `0644` → 打不开或权限错
- `O_CREAT` 必须配合第三个参数(权限)

> [!tip] man 手册是宝：`man 2 open` 看系统调用原型/参数/返回值/错误码。写 IO 前先查。
