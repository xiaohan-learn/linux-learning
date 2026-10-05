---
tags: [linux, 速查, 文件IO, 系统调用]
aliases: [文件IO速查, 系统调用IO]
created: 2026-09-30
---

# 速查 · 文件 IO

> 配合 [[M2-韦东山应用篇#5 文件 IO]] 使用。
> 两层：① **标准库** `fopen` 系列（C 已学，缓冲，见 `ch19`）；② **系统调用** `open` 系列（无缓冲，直接进内核，M2 新学）。

## 标准库 IO（C 层，已掌握，`man 3 xxx`）

### 打开/关闭：fopen / fclose

```c
#include <stdio.h>

FILE *fp = fopen("a.txt", "r");   // 成功返回 FILE*，失败返回 NULL
fclose(fp);                        // 必须配对！不关会丢缓冲数据+泄漏
```

**mode 模式表**（第二个参数，字符串）：

| mode | 含义 | 文件不存在 | 文件已存在 |
|---|---|---|---|
| `"r"`  | 只读 | 报错(NULL) | 从头读 |
| `"r+"` | 读+写 | 报错(NULL) | 从头读写 |
| `"w"`  | 只写 | **创建** | **清空**！ |
| `"w+"` | 读+写 | **创建** | **清空**！ |
| `"a"`  | 追加写 | **创建** | 从末尾追加（写永远在末尾）|
| `"a+"` | 读+追加 | **创建** | 读从头、写到末尾 |

> 加 `b` 即二进制模式：`"rb"` `"wb"` `"ab"`。Linux 下文本/二进制模式**无区别**（不像 Windows 换行转换），但写了可移植性好。

### 读写四件套（参数明细）

```c
// ① 字符级
int fgetc(FILE *fp);              // 读1字符，返回读到值；EOF(=-1)=尾/错
int fputc(int c, FILE *fp);       // 写1字符，成功返回c，失败EOF

// ② 行级（文本专用）
char *fgets(char *buf, int size, FILE *fp);  // 读一行(含\n)，最多size-1字符+补\0
                                             // 返回buf；NULL=尾/错
int fputs(const char *s, FILE *fp);          // 写字符串（不含\0），成功返回>=0

// ③ 格式化
int fprintf(FILE *fp, const char *fmt, ...);  // 用法同 printf，多一个 fp
int fscanf(FILE *fp, const char *fmt, ...);   // 用法同 scanf，多一个 fp

// ④ 整块（二进制专用）
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fp);
//   ptr=数据地址  size=单个元素字节数  nmemb=元素个数  fp=目标
//   返回成功写入的"元素个数"（正常=nmemb，少了就是出错）
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *fp);
//   返回实际读到的元素个数（<nmemb = 尾或错，用 feof/ferror 区分）
```

### 定位与错误

```c
fseek(fp, 0, SEEK_SET);   // 移动位置：SEEK_SET文件头 / SEEK_CUR当前 / SEEK_END末尾
long pos = ftell(fp);     // 查当前偏移（配合 fseek(fp,0,SEEK_END) 可求文件大小）
rewind(fp);               // 等价 fseek(fp, 0, SEEK_SET)

perror("open a.txt");     // 打印"open a.txt: 具体系统错误原因"，最省事的报错方式
```

### 刷新缓冲：fflush（标准库独有）

```c
int fflush(FILE *stream);
//   stream 非空：把该流的"用户态缓冲"立即写入内核
//   fflush(stdout)   → 立刻把 printf 攒着的输出推到屏幕（不用等换行）
//   fflush(NULL)     → 刷新所有打开的输出流
//   成功返回 0，失败返回 EOF
```

> [!warning] ⚠️ `fflush(stdin)` **C 标准未定义行为**：GNU glibc / 微软做了扩展能"丢弃输入缓冲"，但可移植代码别依赖它（清空输入请用 `while(getchar()!='\n');`）。这就是为什么 `fflush` 只在"输出流"上是正规军。
>
> ⚠️ 为什么需要它：标准库 `fprintf/fputs` 默认**行缓冲（终端）/全缓冲（文件）**，数据先攒在用户态 buffer，直到换行 / 写满 / `fclose` 才真正落盘。`open` 系系统调用无此层，每次 `write` 直接进内核——所以 **`fflush` 是标准库 IO 的专属概念**，系统调用用不到。

## 系统调用 IO（应用层重点，`man 2 xxx`，M2 主线）

### 头文件分工（先背）

```c
#include <fcntl.h>    // open 及 O_xxx 宏
#include <unistd.h>   // read / write / close / lseek
```

### open —— 打开/创建文件

```c
int open(const char *pathname, int flags);
int open(const char *pathname, int flags, mode_t mode);  // 带 O_CREAT 时必须三参
// 成功返回 fd（非负整数），失败返回 -1 并设 errno
```

**flags 分三类，用 `|` 按位或组合**：

| 类别            | 宏          | 含义                       |
| ------------- | ---------- | ------------------------ |
| 访问方式（**三选一**） | `O_RDONLY` | 只读                       |
|               | `O_WRONLY` | 只写                       |
|               | `O_RDWR`   | 读写                       |
| 附加行为（可选）      | `O_CREAT`  | 不存在则创建 → **必须带第三参 mode** |
|               | `O_TRUNC`  | 存在则清空                    |
|               | `O_APPEND` | 每次写自动到末尾                 |
|               | `O_EXCL`   | 配合 O_CREAT：已存在则报错（防覆盖）   |

**mode 权限位**（八进制，`ls -l` 那串）：

```
0644  →  拆开看： 6=110(rw-) 所有者    4=100(r--) 同组    4=100(r--) 其他人
常用：0644 普通文件 / 0666 全员读写 / 0600 仅自己
⚠️ 实际权限 = mode & ~umask，所以看到 0644 变 0624 别慌，是 umask 干的
```

### read / write —— 读写（mycp 的灵魂）

```c
ssize_t read(int fd, void *buf, size_t count);
//   fd=从哪读  buf=读到哪  count=最多读多少字节
//   返回值三态：>0 实际读到的字节数（可能 < count！）
//              =0 已到文件尾 EOF ← 循环退出条件
//              -1 出错（errno 说明原因）
ssize_t write(int fd, const void *buf, size_t count);
//   返回实际写入字节数；正常应 == count，不等就是出错/写满
```

> **为什么读返回值可能小于 count**：普通磁盘文件一般能读满；但管道/终端/网络/socket 常常来多少给多少。所以**永远用返回值决定写多少**：`write(out, buf, n)`，n 是 read 的返回值。

### close / lseek

```c
int close(int fd);    // 成功 0，失败 -1；不关 → fd 泄漏（进程退出才强制回收）
off_t lseek(int fd, off_t offset, int whence);
//   whence: SEEK_SET 从头 / SEEK_CUR 当前 / SEEK_END 从尾
//   lseek(fd, 0, SEEK_END) 返回值 = 文件大小（面试小技巧）
```

### errno / perror —— 出错怎么办

```c
if ((fd = open("a.txt", O_RDONLY)) == -1) {
    perror("open a.txt");   // 自动读 errno，打印 "open a.txt: No such file or directory"
    exit(1);
}
```

> [!note] 规矩：**每个系统调用返回值都要查**。open/read/write/close 全可能失败。判 `-1` + `perror` 是 M2 起的标准起手式。

### 强制落盘：fsync / fdatasync（内核 → 磁盘）

```c
#include <unistd.h>
int fsync(int fd);          // 把 fd 相关的"已写内核数据 + 元数据"强制刷到磁盘
int fdatasync(int fd);      // 只刷数据，不刷元数据（atime/mtime 等），更快
//   成功 0，失败 -1；数据库/日志这类"掉电不能丢"的场景才需要
```

> [!warning] **为什么需要它（两层缓冲模型）**：
> - `write()` 返回 ≠ 数据已到磁盘！它只是把数据**拷进内核页缓存**（page cache），由内核后台择机落盘。
> - `fsync(fd)` 才强制内核立刻把缓存**刷到物理磁盘**，返回前保证掉电不丢。
> - 对照 `fflush`（标准库）：`fflush` 是把**用户态缓冲 → 内核页缓存**；`fsync` 是把**内核页缓存 → 磁盘**。两者解决的是不同层：
>
> ```
> 你的数据 →[fflush]→ 用户态缓冲 →[write]→ 内核页缓存 →[fsync]→ 磁盘
> ```
>
> 普通程序一般**不用** fsync（内核会自己刷，且掉电是小概率）；只有"写日志/数据库事务/配置落盘"这种丢一条就出大事的才显式调用。
>
> 全局版 `sync()`（无参数）刷新**所有**脏页，通常只有 `shutdown` 流程才用。

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

### 函数对照表（一层一个角色）

| 需求 | 标准库 (stdio.h) | 系统调用 (unistd.h) |
|---|---|---|
| 打开 | `fopen(path, mode)` | `open(path, flags, mode)` |
| 关闭 | `fclose(fp)` | `close(fd)` |
| 读字符 | `fgetc(fp)` | `read(fd, &c, 1)` |
| 读一行 | `fgets(buf, n, fp)` | （自己实现找 `\n`）|
| 读一块 | `fread(ptr, size, n, fp)` | `read(fd, ptr, bytes)` |
| 写一块 | `fwrite(ptr, size, n, fp)` | `write(fd, ptr, bytes)` |
| 定位 | `fseek(fp, off, whence)` | `lseek(fd, off, whence)` |
| 格式化 | `fprintf / fscanf` | （无，需 snprintf+write）|
| 刷新缓冲 | `fflush(stream)`（→内核页缓存）| （无用户态缓冲）|
| 强制落盘 | （无，fclose 只刷到内核）| `fsync(fd)`（内核→磁盘）/ `sync()`（全局）|
| 报错 | `perror` / `ferror` | `perror`（errno）|

> [!tip] 记法：**标准库多"格式与缓冲"，系统调用多"裸字节与 fd"**。返回值类型也不同：fread/fwrite 返回"元素个数"，read/write 返回"字节数"——混用时最容易踩。

## 实现一个 cp 命令（M2 验收练习 · 全程判错版）
```c
// cp src dst —— 每个系统调用都查返回值
int in = open(argv[1], O_RDONLY);
if (in == -1) { perror("open src"); exit(1); }

int out = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0644);
if (out == -1) { perror("open dst"); exit(1); }

char buf[4096];   // 4K 一块，别 1 字节慢慢磨
ssize_t n;
while ((n = read(in, buf, sizeof(buf))) > 0) {
    if (write(out, buf, n) != n) { perror("write"); exit(1); }  // 读多少写多少
}
if (n == -1) { perror("read"); exit(1); }   // n==0 是正常 EOF，别当错误！

close(in); close(out);
```

## 易错点（对照 C 仓库 BUG.md）
- `open` 失败返回 `-1`，必须判错再继续
- `read` 一次不一定读满 `sizeof(buf)`（尤其设备或网络）→ 循环读
- 忘记 `close` → 文件描述符泄漏
- 创建文件忘给权限 `0644` → 打不开或权限错
- `O_CREAT` 必须配合第三个参数(权限)

> [!tip] man 手册是宝：`man 2 open` 看系统调用原型/参数/返回值/错误码。写 IO 前先查。
