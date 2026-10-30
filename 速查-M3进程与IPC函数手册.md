# 速查：M3 进程与 IPC 函数手册

> **覆盖范围**：进程基础（fork / wait / exec）→ 匿名管道 → 命名管道 FIFO → 共享内存 mmap
> **配套文件 IO**（open / read / write / lseek 等 M2 内容，IPC 全程依赖）单列第 7 节
> **结构约定**：每个函数给「原型 / 头文件 / 参数 / 返回值 / 注意点 / 易错点」六段，末尾第 8、9 节是实战血泪与工具箱；**第 10 节信号函数为 M3D6 进行中骨架**（原型已填准，注意点/易错点标 ⏳ 待学完补全）
> **铁律不变**：全程 `-Wall -Wextra` 零警告；检查每个系统调用的返回值；验证靠工具不靠猜

---

## 0. 概念速览（背诵级）

| 概念 | 一句话 | 关键证据（本课程已实测） |
|---|---|---|
| **程序 vs 进程** | 程序是躺在磁盘的静态文件；进程是运行中的实体（PCB + 地址空间） | 同一份 `ls` 可执行文件，可同时跑出多个进程 |
| **fork + COW** | fork 复制父进程地址空间，但物理页**写时复制**——先共享、谁写谁独 | 父子打印全局 `var`：**地址同 `0x55..8010`、值一个 100 一个 200** |
| **僵尸进程** | 子退出、父不回收，子变成 `Z`（defunct）占着 PID 表项 | `ps ajx` 见 `Z+ [zoom] <defunct>`；kill 父后 init 收尸 |
| **孤儿进程** | 父先死，子被 init（WSL 是 `/init`，pid 1）收养 | 子的 ppid 从父 PID → **28171（/init）** |
| **IPC 四大件进度** | 管道✅ FIFO✅ 共享内存(mmap)✅ 信号✅（M3D6 完成 kill/alarm/setitimer/signal/sigaction/信号集；M3D7 用 SIGCHLD 回收子进程进阶） | — |
| **线程模块进度** | 基础原语✅（M3D8：create/self/join/exit/detach）· 互斥锁✅（M3D9：race 竞态 → mutex 修复）· try 锁/死锁/读写锁/条件变量/信号量 ✅（M3D10 + M3D11 完成） | mutex：并发无锁 **208515** vs 加锁 **500000** |
| **管道读写行为 2×2** | 读空阻塞 / 写端全关返回 0(EOF) / 写满阻塞 / 无读端写→SIGPIPE | 64k 实测卡满；`echo $?` 得 **141 = 128+13** |

---

## 1. 进程创建与退出

### 1.1 `fork` — 创建子进程
```c
#include <unistd.h>
pid_t fork(void);
```
- **参数**：无
- **返回值**：
  - 父进程中返回 **子进程 pid（>0）**
  - 子进程中返回 **0**
  - 失败返回 **-1**（errno 设置）
- **注意点**：
  - 调用一次、返回两次，三段式 if：`if(pid==-1)` 失败 / `else if(pid==0)` 子 / `else` 父（口诀：**0 是子、正数是父**）
  - 子进程继承父的打开文件、信号处理方式、地址空间（COW）
  - 父子执行顺序**不确定**，不要假设谁先跑
  - 循环创建 n 个子：`for(i=0;i<n;i++){ pid[i]=fork(); if(pid[i]==0) break; }` —— 子用 `break` 跳出循环，`i` 即兄弟身份编号
- **易错点**：
  - 在子分支写 `int var=200;` 想“验证 COW”其实是**局部变量遮蔽全局**，证明不了 COW；要改的是全局 `var` + 打印 `&var`
  - 顺序写错（先判 `==0` 再判 `>0` 尚可，但漏判 `-1` 会漏掉失败分支）

### 1.2 `getpid` / `getppid` — 取进程/父进程 ID
```c
#include <sys/types.h>
#include <unistd.h>
pid_t getpid(void);   // 调用进程的 pid
pid_t getppid(void);  // 调用进程的父进程 pid
```
- **参数**：无
- **返回值**：pid_t 类型的进程号（Linux 上即 int，打印用 `%d` 或 `(int)` 强转）
- **注意点**：父的 `getppid` 是 shell（bash），子的 `getppid` 是父——验证父子关系靠这个
- **易错点**：发信号时手敲 pid 易错；用程序自己 `printf("%d",getpid())` 抄，或 `pidof demo` / `pgrep demo` 查

### 1.3 `exit` / `_exit` — 终止进程
```c
#include <stdlib.h>
void exit(int status);     // 刷新 stdio 缓冲区、执行 atexit、再进内核
#include <unistd.h>
void _exit(int status);    // 直接进内核，不刷 stdio 缓冲区
```
- **参数**：`status` 退出码（0 成功，非 0 自定义/错误；仅低 8 位有效，即 0~255）
- **返回值**：不返回
- **注意点**：
  - 两者的差仅在 stdio 缓冲：fork 之前若缓冲区有未刷内容，`_exit` 会让子进程带着副本、父子各刷一遍（重定向全缓冲时 `before fork` 打两遍的现象就是它）
  - 错误路径用 `exit(1)` 比 `return 1` 更规范
- **易错点**：混淆二者导致 fork 前缓冲内容重复输出——理解即可，不必硬改

---

## 2. 进程回收

### 2.1 `wait` — 阻塞回收任一子进程
```c
#include <sys/types.h>
#include <sys/wait.h>
pid_t wait(int *status);
```
- **参数**：`status` 输出子进程退出信息（传 `NULL` 表示不关心）
- **返回值**：成功返回**终止的子进程 pid**；失败返回 **-1**
- **注意点**：等价于 `waitpid(-1, &status, 0)`——阻塞直到任一子进程结束
- **回收宏（配 status 用）**：

| 宏                     | 含义                   |
| --------------------- | -------------------- |
| `WIFEXITED(status)`   | 子是否正常退出（return/exit） |
| `WEXITSTATUS(status)` | 正常退出时的退出码（配合上一宏）     |
| `WIFSIGNALED(status)` | 子是否被信号杀死             |
| `WTERMSIG(status)`    | 若是信号杀死，返回信号编号        |

- **易错点**：
  - 想取退出码却传了 `NULL` → 取不到
  - 把“wait 返回 -1”和“子被信号杀”混淆：返回 -1 是**回收本身出错**，子被信号杀时 wait 仍成功返回 pid、用 `WIFSIGNALED/WTERMSIG` 判断

### 2.2 `waitpid` — 精确/非阻塞回收
```c
#include <sys/types.h>
#include <sys/wait.h>
pid_t waitpid(pid_t pid, int *status, int options);
```
- **参数**：
  - `pid`：`-1`=回收任意子进程；`>0`=回收指定 pid 的子；`0`=同进程组的子
  - `status`：同 wait（传 `NULL` 不关心）
  - `options`：`0`=阻塞；`WNOHANG`=非阻塞（立刻返回）
- **返回值**：
  - `>0`：成功回收的子进程 pid
  - `0`：**仅 WNOHANG 下**表示“子还活着，没死”——**不是出错**
  - `-1`：出错
- **注意点**：`waitpid(pid,NULL,0)` 是 sleep(1) 的正规替代——真阻塞等到子结束，不再靠“赌 1 秒够用”
- **易错点（致命）**：
  - **WNOHANG 返回 0 当 error 退出 → 漏收**；它只回答“问的那一瞬间”，要结果必须**循环里每轮重新调用**（`ret=waitpid(...)` 写在 `while(1)` 内第一行），否则“只问一次”会死循环刷屏
  - `pid` 传 `-1` 是“谁先死收谁”，多个子时要 `>0` 指定才精准

### 2.3 回收宏（见 2.1 表格，实战用例）
```c
int status;
pid_t w = waitpid(pid, &status, 0);
if (WIFEXITED(status))  printf("正常退出，码=%d\n", WEXITSTATUS(status));
if (WIFSIGNALED(status)) printf("被信号杀，信号=%d\n", WTERMSIG(status));
```

---

## 3. 程序替换 exec 族

### 3.1 `execl` / `execlp` — 替换进程映像
```c
#include <unistd.h>
int execl(const char *path, const char *arg, ... /*, (char *)NULL */);
int execlp(const char *file, const char *arg, ... /*, (char *)NULL */);
```
- **参数**：
  - `path`：可执行文件**全路径**（execl）
  - `file`：文件名（execlp 从 `PATH` 环境变量里找）
  - `arg, ...`：新程序的 argv 列表，**argv[0] 习惯写成程序名**，参数依次跟，列表以 **`(char*)NULL`** 结尾
- **返回值**：**成功不返回**（返回值是 -1 仅表示失败）；失败返回 **-1**（errno 设置）
- **注意点**：
  - 替换的是**进程映像**（代码/数据/堆/栈），PID 不变——“fork 造壳 + exec 换核”
  - `execlp("ls","ls",NULL)` 与 `execl("/bin/ls","ls",NULL)` 区别只在于找路径的方式
- **易错点**：
  - **execl 成功后后续代码是死代码**——`close(fd)` / `wait(NULL)` 写在 execl 之后永远执行不到；规范写法是 dup2 + close 都放 execl **之前**
  - 末尾忘了 `NULL` 结尾 → 未定义行为
  - 自己设计对照实验：一个用 execlp（走 PATH）、一个用 execl（全路径），体会差异

---

## 4. 匿名管道

### 4.1 `pipe` — 创建匿名管道
```c
#include <unistd.h>
int pipe(int fd[2]);
```
- **参数**：`fd` —— 长度为 2 的 int 数组；`fd[0]`=读端，`fd[1]`=写端
- **返回值**：成功返回 **0**；失败返回 **-1**
- **注意点**：
  - 本质是**内核环形缓冲区**，老内核 4k / 新内核（2.6.11+）**默认 64k**（已实测卡在 65536 字节）
  - **半双工**（单向）；只服务有血缘进程（fork 继承 fd）
  - 口诀“**谁不用关谁**”：父只写→关 fd[0]；子只读→关 fd[1]
- **易错点**：
  - 兄弟管道里**父不关自己的 fd[0] 和 fd[1]** → 残留写端让读端永远读不到 EOF（卡死）
  - 管道数据“**读走即消失**”，多个读者瓜分同一份（非广播）
  - `read` 返回 **0 = 写端全关（EOF）**，不是错误（见第 7.2）
  - 管道**写满阻塞**；**无读端还写 → SIGPIPE 杀进程（退出码 141）**

### 4.2 `dup2` — 复制/重定向文件描述符
```c
#include <unistd.h>
int dup2(int oldfd, int newfd);
```
- **参数**：把 `newfd` 重定向到 `oldfd`（先关闭 newfd 再复制）
- **返回值**：成功返回 `newfd`；失败返回 **-1**
- **注意点**：`ls | wc -l` 的复刻核心——`dup2(fd[1], STDOUT_FILENO)` 把管道写端嫁接到标准输出，execl 后 ls 以为在写屏幕、其实写进管道；wc 侧 `dup2(fd[0], STDIN_FILENO)` 同理
- **易错点**：dup2 之后 `close(oldfd)` 要放在 execl **之前**（之后是死代码）

---

## 5. 命名管道 FIFO

### 5.1 `mkfifo` — 创建命名管道
```c
#include <sys/types.h>
#include <sys/stat.h>
int mkfifo(const char *pathname, mode_t mode);
```
- **参数**：`pathname` 管道文件路径；`mode` 权限（如 `0644`）
- **返回值**：成功返回 **0**；失败返回 **-1**
- **注意点**：
  - 创建的是文件系统里的 **`p` 类型特殊文件**（本质是内核缓冲区，磁盘上 **0 字节**，数据不在磁盘）
  - 实现**无血缘**进程通信的握手：两端都 `open` 才不阻塞（只读端 open 等写端、只写端 open 等读端）
- **易错点**：
  - 不处理 `errno==EEXIST` → 第二次运行 `mkfifo` 报错退出；规范写法 `if(mkfifo(...)==-1 && errno!=EEXIST){ perror; exit(1); }`
  - 读端不该带 `O_TRUNC` / `ftruncate`：会把写端已写好的数据抹掉（读端“只读不截”）
  - 误以为 FIFO 文件有内容：`cat` 它看到 0 字节是正常的

---

## 6. 共享存储映射（mmap）

### 6.1 `mmap` — 建立映射区
```c
#include <sys/mman.h>
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
```
- **参数**：
  - `addr`：期望映射起始地址，通常传 **`NULL`** 让内核选
  - `length`：映射长度（**必须 ≤ 文件大小**，否则写越界 → SIGBUS）
  - `prot`：`PROT_READ` / `PROT_WRITE` / `PROT_EXEC` 按位或
  - `flags`：`MAP_SHARED`（改动写回文件 + 跨进程可见）/ `MAP_PRIVATE`（写时复制，各写各的）/ `MAP_ANONYMOUS`（匿名，无血缘不可用、fd 填 -1）
  - `fd`：映射后端文件的 fd（匿名填 -1）
  - `offset`：文件偏移，**必须页对齐（4k 倍数）**，否则 EINVAL
- **返回值**：成功返回**映射区首地址**；失败返回 **`MAP_FAILED`（即 `(void*)-1`），不是 NULL**
- **注意点**：
  - 映射区当内存用：`memcpy(p, src, len)` 写 = 写文件（MAP_SHARED 会写回）
  - `MAP_SHARED` 跨进程可见：`fork` 前映射 → 子改父读；无血缘则双方 open 同一文件再映射
- **易错点（高频）**：
  - **判 `==NULL` 错，应判 `==MAP_FAILED`**
  - **写映射区要用 `memcpy`，不是 `write()`**——write 走系统调用另一条路（`ls -l` 文件 110B 是 write 假阳性，100B 才是 mmap 真写入）
  - **MAP_PRIVATE 子改父看不见**（父读到的是文件原本的 0），对照 MAP_SHARED（父读到 1000）
  - 映射长度超出文件 → SIGBUS；offset 不对齐 → EINVAL
  - 验证用 `od -c`（看字节落在偏移 0 还是 100）+ `ls -l`（看大小），**`cat` 有字 ≠ 验证通过**

### 6.2 `munmap` — 释放映射区
```c
#include <sys/mman.h>
int munmap(void *addr, size_t length);
```
- **参数**：`addr` 必须是 mmap 的**原返回值**；`length` 必须与映射长度一致
- **返回值**：成功返回 **0**；失败返回 **-1**
- **易错点**：addr 用错指针（如局部变量地址）会 munmap 失败或崩

### 6.3 `ftruncate` — 撑大文件
```c
#include <unistd.h>
int ftruncate(int fd, off_t length);
```
- **参数**：`fd` 打开的文件；`length` 目标大小
- **返回值**：成功返回 **0**；失败返回 **-1**
- **注意点**：比 `lseek(fd,N,SEEK_END)+write(fd,"\0",1)` 更直白，用来给映射区撑出合法长度
- **易错点**：读端也 `ftruncate` 会抹数据；只在写端撑大小

---

## 7. 配套文件 IO（M2 内容，IPC 全程依赖）

### 7.1 `open` / `close`
```c
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
int open(const char *pathname, int flags, mode_t mode);  // 创建时给 mode
int close(int fd);
```
- **open flags**：`O_RDONLY` / `O_WRONLY` / `O_RDWR`；创建类加 `O_CREAT`（配 `mode`，如 `0644`）、`O_TRUNC`（清空）；管道/FIFO 场景**读端别带 O_TRUNC**
- **返回值**：open 成功返回**新的 fd（最小可用非负整数）**；失败返回 **-1**。close 成功 0 / 失败 -1
- **易错点**：open 失败未检查 → 后续 read/write 拿到 -1 fd 静默失败

### 7.2 `read`
```c
#include <unistd.h>
ssize_t read(int fd, void *buf, size_t count);
```
- **返回值**：**>0 读到的字节数**；**0 = 到达 EOF（写端全关）**；**-1 = 出错**
- **注意点**：read 不自动补 `\0`，裸字节写入
- **易错点（高频）**：
  - **`if(rn<=0)` 把 EOF 当错误处理** → perror 打 `Success`（errno 未被设置）；正确：`rn<0` 才是错，`rn==0` 单独 break
  - 读完忘了 `buf[rn]='\0'` → `%s` 打印读到栈上垃圾（运气好才不串）
  - `rn` 是 ssize_t（有符号），数组下标注意无符号问题

### 7.3 `write`
```c
#include <unistd.h>
ssize_t write(int fd, const void *buf, size_t count);
```
- **返回值**：**>0 写入字节数**；**-1 = 出错**（如 EBADF 写已关闭 fd、SIGPIPE 无读端）
- **注意点**：和 mmap 的 memcpy 是两条路，别混用（见 6.1）
- **易错点**：**不检查返回值** → EBADF/SIGPIPE 静默失败（写已关 fd 时尤其隐蔽）；写入长度用 `strlen`（内容长度）而非 `sizeof`（容器长度，会多写 `\0`）

### 7.4 `lseek`
```c
#include <unistd.h>
off_t lseek(int fd, off_t offset, int whence);
```
- **whence**：`SEEK_SET`（从文件头）/ `SEEK_CUR`（当前）/ `SEEK_END`（末尾）
- **返回值**：成功返回**新的文件偏移**；失败返回 **`(off_t)-1`**
- **注意点**：`open` 后 `lseek(fd,0,SEEK_END)` 可拿到文件当前大小
- **易错点**：函数名漏写、写成逗号表达式导致语句失效 → 返回值恒等于常量（见 M2 易错点 13）

---

## 8. 实战血泪易错点速记（M3 专属）

| # | 现象 | 根因 | 防法 |
|---|---|---|---|
| 1 | 子写 `int var=200` 以为验证了 COW | 局部变量遮蔽全局，与 fork 无关 | 改全局 var + 打印 `&var`，地址同值异才是 COW |
| 2 | waitpid + WNOHANG 死循环刷屏 | `waitpid` 写在 `while` 外，只问一次 | 把 `ret=waitpid(...)` 放进循环第一行，每轮重问 |
| 3 | 兄弟管道读端卡死 | 父没关自己的 fd[0]/fd[1]，残留写端让 EOF 不来 | 父 fork 后两端都 close |
| 4 | 管道读端 `read: Success` 刷不停 | 把 EOF(0) 当错误送 perror | `rn<0` 才 perror，`rn==0` 单独 break |
| 5 | 管道写满卡在 65536 | 写满阻塞（正常行为） | 知道 64k 是实测上限；读端及时读 |
| 6 | 无读端写管道程序“安静退出” | SIGPIPE(13) 默认杀进程，连 perror 都来不及 | `echo $?` 得 141=128+13；信号章再学 signal 忽略 |
| 7 | FIFO 第二次跑 `File exists` | mkfifo 对已存在文件返回 EEXIST | `if(mkfifo(...)==-1 && errno!=EEXIST)` 放行 |
| 8 | mmap `cat` 有字但其实是 write 写的 | 用了 `write(fd,...)` 而非 `memcpy(p,...)` | 写映射区用 memcpy；用 `ls -l`/`od -c` 验位置 |
| 9 | mmap 判 `==NULL` 拦不住失败 | 失败返回 `MAP_FAILED` 即 `(void*)-1` | 判 `==MAP_FAILED` |
| 10 | 无血缘 mmap 读端读到全 0 | 读端带了 `O_TRUNC`/`ftruncate` 抹数据 | 读端只读不截；先 `./w` 再 `./r` |
| 11 | 八进制坑 `stu={001,...}` | 前导 0 整数字面量按八进制解析 | 十进制直接写 `1` |
| 12 | execl 后 close/wait 不执行 | execl 成功不返回，后续是死代码 | dup2/close 放 execl 之前 |

---

## 9. 验证 / 调试工具箱

| 工具 | 用途 | 典型命令 |
|---|---|---|
| `-Wall -Wextra` | 编译期拦截拼写/类型/括号类错误 | `gcc -Wall -Wextra x.c -o x` |
| `echo $?` | 看上一个程序退出码（141=SIGPIPE） | `./x; echo $?` |
| `od -c` | 字节级看内容落在哪个偏移（验 mmap 位置） | `od -c test.txt \| head` |
| `ls -l` | 看文件大小（110B=write 假阳性 / 100B=mmap 真写入） | `ls -l test.txt` |
| `ps` / `ps ajx` | 看进程状态 Z(僵尸)/T(暂停)/S(运行) | `ps ajx \| grep zoom` |
| `kill -l` | 看全部 62 个信号（无 32/33） | `kill -l` |
| `man 2 xxx` / `man 7 signal` | 查函数原型 / 信号默认动作与事件 | `man 2 mmap`、`man 7 signal` |
| `pidof` / `pgrep` | 查进程 pid（发信号别手敲） | `pidof demo` |
| `pstree -p` | 看进程树 | `pstree -p \| grep x` |

---

> **本笔记对应仓库实战**：`practice/M3D1_fork/`（fork/wait/exec/僵尸/孤儿）、`practice/M3D2_ipc/`（waitpid 轮询 + 匿名管道 + `ls|wc -l`）、`practice/M3D3_ipc/`（兄弟管道 + 64k 实测 + FIFO 双终端）、`practice/M3D4_mmap/`（mmap 文件映射 + 双 flag 对照 + 无血缘结构体通信）。
> **下一步（M3D5/D6）**：信号章——kill/alarm/setitimer/信号集函数/sigaction 捕捉，收尾 IPC 第四件。

---

## 10. 信号函数（IPC 第四件 · M3D6 进行中，骨架待补全）

> **状态说明**：M3D5 上午已学概念/机制/三态（产生→未决→递达）/三处理方式（默认/忽略/捕捉）/四要素（编号+名称+事件+默认动作）；`kill -l` 见 62 个信号（无 32/33）。本节的 **kill/alarm/setitimer/信号集/sigaction** 属 M3D6 函数实战，**原型与已知要点先列准，⏳ 标注处待学完回填**。
> **已确认硬知识**：`SIGKILL(9)` 与 `SIGSTOP(19)` **不可捕获、不可忽略**（M3D5 上午 `kill -9` 亲验）；其余信号可被捕捉/忽略。

### 10.0 信号四要素速查表（编号 + 名称 + 事件 + 默认动作）

> **四要素** = 编号 + 名称 + 产生事件（触发条件）+ 默认动作。默认动作缩写：**Term**=终止 / **Core**=终止并 core dump / **Stop**=暂停 / **Cont**=继续 / **Ign**=忽略。
> 🧪 = M3D5 上午用 `kill -n <pid>` 在 `sign_demo`（裸 `pause()`）上**亲手验过默认动作**；其余按 `man 7 signal` 标准。

| 编号 | 名称 | 产生事件（触发条件） | 默认动作 | 备注 |
|---|---|---|---|---|
| 2 | `SIGINT` | `Ctrl+C` / 键盘中断 | Term | 🧪 静默终止（2497 直接回到提示符） |
| 3 | `SIGQUIT` | `Ctrl+\` | Core | 🧪 打印 `Quit` 再退出；与 SIGINT 区别：**多一份 core dump** |
| 9 | `SIGKILL` | `kill -9` 管理员强杀 | Term | 🧪 **不可捕获、不可忽略**，连 handler 都拦不住 |
| 18 | `SIGCONT` | `kill -18` 继续运行 | Cont | 🧪 与 SIGSTOP 配对，恢复被暂停的进程 |
| 19 | `SIGSTOP` | `Ctrl+Z` / 作业控制暂停 | Stop | 🧪 **不可捕获、不可忽略**（`kill -19` 后状态变 T） |
| 15 | `SIGTERM` | `kill` 无参默认信号 | Term | 优雅终止；可被捕捉，给程序清场机会（区别于 9） |
| 14 | `SIGALRM` | `alarm()` / `setitimer` 定时到点 | Term | 配合 10.2/10.3 捕捉做定时器 |
| 17 | `SIGCHLD` | 子进程状态改变（退出/暂停/继续） | Ign | 默认忽略；捕捉后可异步回收子（见 10.7 进阶） |
| 10 | `SIGUSR1` | 用户自定义 | Term | 应用自定用途 |
| 12 | `SIGUSR2` | 用户自定义 | Term | 应用自定用途 |
| 11 | `SIGSEGV` | 段错误（非法内存访问 / 空指针解引用） | Core | 面试常客 |
| 13 | `SIGPIPE` | 写无人读的管道 / 已关闭的 socket | Term | = M3D3 实测退出码 **141** 那位（128+13） |

**补充（常用但不必背全）**：`SIGHUP(1)` 终端关闭 / 守护进程重读配置；`SIGABRT(6)` `abort()` 自杀（Core）；`SIGTSTP(20)` `Ctrl+Z` 暂停（**可捕获**，区别于不可捕的 SIGSTOP）。
**查全量**：`kill -l`（共 62 个，**无 32/33**）；权威细节 `man 7 signal`。

### 10.1 `kill` / `raise` — 发送信号
```c
#include <sys/types.h>
#include <signal.h>
int kill(pid_t pid, int sig);
int raise(int sig);                 // 等价于 kill(getpid(), sig)
```
- **参数（kill）**：
  - `pid`：`>0` 发给该进程；`=0` 发给同进程组全部；`=-1` 发给有权限的全部进程（不含 init）；`<0` 发给进程组 `|pid|`
  - `sig`：信号编号（如 `SIGINT`/`SIGKILL`）；`sig=0` 用于**探测进程是否存在**（不真正发信号）
- **返回值**：成功返回 **0**；失败返回 **-1**（errno）
- **已知要点**：`kill(getpid(), sig)` 给自己发；`SIGKILL`/`SIGSTOP` 不可捕获（见状态说明）
- **注意点**：
  - **权限边界**：只能给同 uid（或 root）的进程发信号；给别的用户进程发会 `EPERM` 失败返回 -1
  - **`sig=0` 探测进程是否存在**：不真正发信号，仅检查 `pid` 是否可达（可达返回 0，不可达返回 -1 置 `ESRCH`）
  - **`SIGKILL`/`SIGSTOP` 不可捕获**：即使对方装了 handler，`kill(pid, SIGKILL)` 照样秒杀（kill_demo 父进程用 `kill(pid, SIGKILL)` 强杀子，子任何 handler 都拦不住）
  - 子进程必须**先** `signal(SIGUSR1, handler)` 注册，父 `kill(pid, SIGUSR1)` 发出后才会进 handler 打印（已亲验 "child got SIGUSR1"）
- **易错点**：
  - **信号号必须和对方注册的一致**：kill 发 SIGUSR1、对方却只捕获 SIGINT，则走默认动作（终止），不会进你的 handler
  - **忘记 `wait` 回收**：父 kill 掉子后必须 `wait(NULL)`，否则子变僵尸（kill_demo 已正确回收）
  - 勿误杀：确认 `pid` 正确，别把父进程或自己 kill 了
  - 返回值别漏判 -1

### 10.2 `alarm` — 一次性定时器
```c
#include <unistd.h>
unsigned int alarm(unsigned int seconds);
```
- **参数**：`seconds` 秒后触发 `SIGALRM(14)`；`seconds=0` 取消已有定时器
- **返回值**：**之前设定闹钟的剩余秒数**（首次设返回 0）
- **已知要点**：进程只维护**一个**闹钟，重复调用覆盖之前的；到点默认终止进程（除非捕捉 SIGALRM）
- **注意点**：
  - 进程只维护**一个**闹钟，重复调用覆盖前一个；返回值是"上一个闹钟的剩余秒数"
  - **默认动作是终止进程**——不捕获时 shell 打印 `Alarm clock` 后退出（alarm.c 亲验：1 秒后 "Alarm clock"）；想做别的事必须 `signal(SIGALRM, handler)` 捕获
  - 与 `sleep` 互相干扰：alarm 会打断 sleep，sleep 底层也用 SIGALRM，二者别混用
  - 子进程**不继承**父的 alarm（fork 后子进程闹钟被清零）
- **易错点**：
  - ⚠️ **注册信号号写错**：`alarm()` 发的是 `SIGALRM`，若误写成 `signal(SIGUSR1, handler)`（alarm_demo.c 原版 bug），handler 永远不触发，到点后进程照常被 SIGALRM 默认动作杀死、终端打印 "Alarm clock" 而非"时间到"。**正确：`signal(SIGALRM, handler)`**（setitimer_demo.c 即正确版，亲验打印"第N次触发SIGALRM"）
  - "Alarm clock" 出现在屏幕而非重定向文件：这是 shell 对"子进程死于 SIGALRM"的提示，走终端不进 stdout，所以 `./alarm > out` 仍看到 Alarm clock、数字进 out

### 10.3 `setitimer` — 周期/高精度定时器
```c
#include <sys/time.h>
int setitimer(int which, const struct itimerval *new_value,
              struct itimerval *old_value);
struct itimerval {
    struct timeval it_interval;   // 周期（两次触发间隔）；全 0 = 一次性
    struct timeval it_value;      // 首次触发时间
};
struct timeval { long tv_sec; long tv_usec; };   // 秒 + 微秒
```
- **参数**：
  - `which`：`ITIMER_REAL`（真实时间→`SIGALRM`）/ `ITIMER_VIRTUAL`（用户态时间→`SIGVTALRM`）/ `ITIMER_PROF`（用户+内核→`SIGPROF`）
  - `new_value`：新的定时设置；`old_value` 输出旧设置（传 `NULL` 不关心）
- **返回值**：成功返回 **0**；失败返回 **-1**
- **已知要点**：相比 `alarm` 支持微秒级 + 周期性（`it_interval` 非 0 则循环触发）
- **注意点**：
  - `which=ITIMER_REAL` 最常用：真实时间，到点发 `SIGALRM`；另有 `ITIMER_VIRTUAL`（用户态时间→SIGVTALRM）、`ITIMER_PROF`（用户+内核→SIGPROF）
  - `it_value` = 首次触发延时；`it_interval` = 后续周期（**全 0 = 只触发一次**做一次性；非 0 = 周期性）
  - 必须在 `setitimer` **之前** `signal(SIGALRM, handler)` 注册好 handler，否则首次 SIGALRM 走默认（终止）
  - 比 `alarm` 强：支持微秒级（`tv_usec`）+ 周期性
- **易错点**：
  - 结构体**显式赋全字段**（`it_interval`/`it_value` 的 `tv_sec`/`tv_usec` 都设），别留未初始化的脏值（setitimer_demo.c 全部显式赋值，零警告跑通）
  - 同样要捕获 `SIGALRM`：误注册成 SIGUSR1 则 handler 永不触发（与 alarm_demo bug 同源）
  - handler 里 `exit(0)` 是演示写法，真实程序慎用（直接结束进程、跳过清理）；更稳妥是置标志位让主循环处理

### 10.4 `signal` — 简单信号捕捉
```c
#include <signal.h>
typedef void (*sighandler_t)(int);
sighandler_t signal(int signum, sighandler_t handler);
```
- **参数**：
  - `signum`：要捕捉的信号编号
  - `handler`：`SIG_IGN`（忽略）/ `SIG_DFL`（默认）/ 自定义函数 `void handler(int sig)`（捕获后调用，参数为信号号）
- **返回值**：成功返回**旧的处理函数指针**；失败返回 **`SIG_ERR`**（不是 NULL）
- **已知要点**：入门级捕捉接口，写法简单
- **注意点**：
  - handler 原型 `void handler(int sig)`，参数 `sig` 是收到的信号号——一个 handler 可服务多个信号，靠 `sig` 区分（setitimer_demo.c 即 `if(sig==SIGALRM)`）
  - 返回值：成功返回**旧** handler 指针；失败返回 **`SIG_ERR`**（不是 NULL，判错用 `==SIG_ERR`）
- **易错点**：
  - ⚠️ **不可靠语义**：`signal` 在部分系统上 handler 执行后会**自动复位为默认**（`SA_RESETHAND` 风格），导致第二次同信号走默认动作；**生产用 `sigaction`**（见 10.5）
  - 编译警告 `unused parameter 'sig'`：handler 没用到 sig 参数时 `-Wextra` 会报；可在 handler 首行 `(void)sig;` 静音（kill_demo.c / alarm_demo.c 都有此警告；setitimer_demo.c 因用了 sig 而零警告）
  - **handler 内慎用非可重入函数**：`printf` 在教材 demo 常用，但严格说不是 async-signal-safe；正式代码用 `write(STDOUT_FILENO, ..., n)` 或只在 handler 里置 `volatile sig_atomic_t` 标志、主循环处理

### 10.5 `sigaction` — 健壮信号捕捉
```c
#include <signal.h>
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
struct sigaction {
    void (*sa_handler)(int);                    // 简单处理函数
    void (*sa_sigaction)(int, siginfo_t *, void *); // 带信息的处理函数（sa_flags 含 SA_SIGINFO）
    sigset_t sa_mask;                           // 执行 handler 期间额外屏蔽的信号集
    int sa_flags;                               // SA_RESTART / SA_SIGINFO / SA_RESETHAND ...
};
```
- **参数**：`signum` 信号号；`act` 新的处理方式（传 `NULL` 表示不改只查询）；`oldact` 输出旧方式（`NULL` 不关心）
- **返回值**：成功返回 **0**；失败返回 **-1**
- **已知要点**：比 `signal` 可控——`sa_mask` 指定 handler 执行期间自动屏蔽哪些信号，`sa_flags` 控制行为（如 `SA_RESTART` 自动重启被中断的慢系统调用）
- **注意点**：
  - `sa_handler` 与 `sa_sigaction` 二选一：基础用 `sa_handler`（只收信号号）；要拿发送者 pid / 附带数据才用 `sa_sigaction`（需 `sa_flags|=SA_SIGINFO`）
  - `sa_mask`：handler 执行期间**额外**屏蔽的信号集；而 `signum` 自身在 handler 运行时会**被自动屏蔽**（不用写进 sa_mask）——这正是 sigact_demo 里"第一次 handler 的 sleep 期间再按 Ctrl+C 被卡住、handler 结束后才补递达"的原因
  - `sa_flags=0`：默认**不**自动重启被中断的慢系统调用（read 等会返回 -1/EINTR）；置 `SA_RESTART` 可让内核自动重启该系统调用（免手写 EINTR 重试循环）
  - 返回值 0/-1，失败 `perror("sigaction")` 退出
  - **不可用于 `SIGKILL`/`SIGSTOP`**：任何 handler 对它们都无效
- **易错点**：
  - ⚠️ **标准信号不排队**：sigact_demo 中 handler 里 `sleep(5)`，期间连按多次 Ctrl+C，只有**一次**会在 handler 结束后补递达（标准信号 pending 只记"有没有"，不记"几次"）；实时信号 `SIGRTMIN+` 才排队——实战/面试常考
  - **`sigaction` 比 `signal` 可靠**：`signal` 在部分系统 handler 执行后会自动复位为默认（第二次同信号走默认动作），`sigaction` 不会；sigact_demo 连按三次 Ctrl+C 都稳定进 handler 即证
  - handler 内慎用非可重入函数（`printf` 等）；正式代码用 `write(STDOUT_FILENO,...)` 或只在 handler 里置 `volatile sig_atomic_t` 标志、主循环处理

### 10.6 信号集函数（sigsetops）+ 屏蔽字/未决集
```c
#include <signal.h>
int sigemptyset(sigset_t *set);              // 清空信号集（全 0）
int sigfillset(sigset_t *set);               // 填满信号集（全 1）
int sigaddset(sigset_t *set, int signum);    // 把某信号加入集
int sigdelset(sigset_t *set, int signum);    // 把某信号移出集
int sigismember(const sigset_t *set, int signum); // 判断是否在集中（1 在 / 0 不在 / -1 错）
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);   // 设置/查询进程屏蔽字
int sigpending(sigset_t *set);               // 取“阻塞且未决”的信号集
```
- **sigprocmask 的 `how`**：`SIG_BLOCK`（并集）/ `SIG_UNBLOCK`（移除）/ `SIG_SETMASK`（直接设定）
- **返回值**：本组函数成功返回 **0**（-1 失败）；`sigismember` 特殊：1=在集 / 0=不在 / -1=错
- **已知要点**：对应 M3D5 下午 **04-信号屏蔽字和未决信号集**——`sigprocmask` 操作**阻塞位图**（置 1=屏蔽，收到后卡在未决），`sigpending` 读**未决位图**（产生即翻 1、处理完翻 0）
- **注意点**：
  - `sigemptyset` 必须**先于** `sigaddset` 调用（否则集未初始化，位图是随机值）
  - `sigprocmask(how, set, oldset)`：`SIG_BLOCK`(并集)/`SIG_UNBLOCK`(移除)/`SIG_SETMASK`(直接设定)；`oldset` 可取旧屏蔽字（传 `NULL` 不关心）
  - `sigpending(set)` 取"已产生且被阻塞、尚未递达"的信号集；`sigismember` 逐位查（1 在 / 0 不在），常配合 `for` 循环打印调试
  - 多线程中 `sigprocmask` 只影响**调用线程**；进程级用 `sigprocmask`，线程级用 `pthread_sigmask`
- **易错点**：
  - ⚠️ **屏蔽 ≠ 忽略**：屏蔽只是让信号"暂不递达"，它仍会进入 pending 集、**不丢失**，解除屏蔽立即递达（mask_demo 亲验：pending 打印第 2 位=1，解除后 handler 立刻打印 "hello,sigprocmask"）；而"忽略"是信号被直接丢弃、永不触发
  - pending 集用 `sigpending` 读，别和"屏蔽字"搞混：屏蔽字=我还不想收哪些；未决集=已经来了但我还没收的
  - 打印 pending 是排查"信号好像丢了"的利器——信号其实常在 pending 里等你解除屏蔽

### 10.7 M3D6 后回填清单（勾掉本骨架的 ⏳）
- [x] 10.0 四要素表：12 个核心信号已列（🧪 标出上午亲验 5 个）；待 `man 7 signal` 核对编号/事件/默认动作无偏差
- [x] 10.1 kill：权限边界、pid 取值陷阱、`sig=0` 探测（上午 kill_demo 亲验：发 SIGUSR1 进 handler、SIGKILL 强杀不可捕、wait 回收）
- [x] 10.2 alarm：与 sleep 冲突、子进程不继承（上午 alarm.c 亲验默认 Alarm clock 终止；alarm_demo 原版写错 SIGUSR1 的 bug 已记录）
- [x] 10.3 setitimer：结构体初始化、三种 which 区别、`it_interval=0` 一次性（上午 setitimer_demo 亲验周期 1s×5 次退出、零警告）
- [x] 10.4 signal：自动复位不可靠问题、handler 可重入、unused-parameter 静音（上午四程序统一验证）
- [x] 10.5 sigaction：`sa_mask`/`sa_flags`/`SA_RESTART` 实战、`SIGKILL`/`SIGSTOP` 不可捕、handler 可重入、标准信号不排队（下午 sigact_demo 亲验：三次 Ctrl+C 稳定进 handler、handler 内 sleep 期间再按只补递达一次）
- [x] 10.6 屏蔽字/未决集：阻塞≠忽略、未决信号不丢失、解除即递达（下午 mask_demo 亲验：屏蔽 SIGINT 后 kill 自送 → pending 第2位=1 → 解除后 handler 打印 "hello,sigprocmask"）
- [x] 进阶：借助信号捕捉回收子进程（`SIGCHLD` + `waitpid(WNOHANG)` 循环）（M3D7 child_recycle.c 亲验）
  - 亲验：`fork()` 10 个子（子 `exit(i)` 退码 0~9），父先 `sigaction(SIGCHLD,&act,NULL)` 注册、**再**循环 `fork`；handler 内 `while((wpid=waitpid(-1,&status,WNOHANG))>0){ if(WIFEXITED(status)) printf("recycle child pid=%d, exit code=%d\n",wpid,WEXITSTATUS(status)); }`；运行 10 子全部被回收、`ps ajx` 无 `Z` 僵尸、编译零警告 ✅
  - **为什么必须 `while` 循环**：SIGCHLD 是**标准信号不排队**，10 个子几乎同时退出只触发**一次** handler，单次 `waitpid` 会漏回收 → 其余变僵尸（`Z`）；循环把这次"积压"的全部回收干净
  - **必须用 `WNOHANG`**：handler 内禁用阻塞 `wait()`（没子可回收时会卡死父进程）；`WNOHANG` 非阻塞，没子可回收时立即返回 0 跳出循环
  - **`WIFEXITED/WEXITSTATUS` 只决定打印什么、不影响回收**：循环里的 `waitpid` 无论子正常退出还是被信号杀都会回收它（不漏僵尸）；只是被信号杀时 `WIFEXITED` 为假、不打印 exit code 而已
  - **注册顺序**：父先 `sigaction` 装好 handler 再 `fork`，子继承后 `exit` 给父发 SIGCHLD → 父 handler 回收；顺序反了可能漏掉早期子
  - **对比 `signal(SIGCHLD, SIG_IGN)`（M3D7 child_ign.c 亲验）**：`act.sa_handler=SIG_IGN` 后 `sigaction(SIGCHLD,&act,NULL)`，fork 10 子 exit(i)，父 `sleep(2)` 后退出；运行**无任何 "recycle" 打印**、`ps ajx` **无 Z 僵尸** ✅——证明显式忽略时内核**自动回收**子进程、不产生僵尸，但**拿不到退出状态**（无 handler 可读 status）
  - ⚠️ **关键易混：SIGCHLD 默认动作就是 Ign，但"默认 Ign"≠"显式 SIG_IGN"**：
    - **默认（什么都不做）**：SIGCHLD 默认动作虽是 Ign，但内核**不会**自动回收子进程 → 子退出变僵尸（Z），必须父 `wait()`——这就是 M3D2/D3 不 wait 就见 Z+ 的原因
    - **显式 `signal(SIGCHLD,SIG_IGN)` / `sigaction` 设 `SIG_IGN`**：这是 **Linux/SysV 特殊语义**，内核会在子退出时**立即自动回收**、不产生僵尸
    - 二者行为天差地别，面试/实战常考；可移植代码不应依赖"显式 IGN 自动回收"，需要退出状态时一律用**捕捉版**（child_recycle.c）
- [ ] 进阶：慢速系统调用被信号中断（`SA_RESTART`）（10.5 已讲理论；待 TCP 服务器实战验证 EINTR 处理）
- [ ] 把信号相关易错点补进第 8 节实战血泪表

---

## 11. 线程（pthread）基础原语 ⭐ M3D8 新增

> **统一规则**：头文件 `<pthread.h>`；**编译必须 `-pthread`**（pthread 不在 libc，漏了报 `undefined reference to pthread_create`；用 `-pthread` 而非 `-lpthread`，前者还定义 `_REENTRANT` 让 libc 走可重入版本）；**pthread 系列函数成功返回 0、失败返回错误码且不设置 errno** —— 判 `ret != 0` 后用 `strerror(ret)` 打错，**不能用 perror**。

### 11.1 pthread_create（创建线程）
```c
#include <pthread.h>
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);
```
- **参数**：`thread` 出参收新线程 tid；`attr` 线程属性（`NULL`=默认 JOINABLE）；`start_routine` 线程函数（**签名必须 `void* fn(void*)`**）；`arg` 传给线程函数的参数
- **返回值**：成功 **0**；失败返回**错误码**（不设 errno、不写 thread）
- 🧪 **亲验要点**（pthread_create.c）：
  - 5 线程 + 主线程 **pid 全相同**（同进程共享地址空间）、tid 各不同 —— 线程 vs fork 的根本区别铁证
  - 传参用 `(void*)(long)i` **传值**，线程内 `long i = (long)arg` 取回；传 `&i` 地址会踩"循环快、线程慢拿到错值"坑
  - 线程函数漏 `return NULL` → 警告 control reaches end of non-void function
- **易错点**：
  - 漏 `-pthread` → `undefined reference to pthread_create`（新人第一大错）
  - 签名写错（`void fn()` / `int fn()`）→ 类型不兼容 `void*(*)(void*)`
  - 取参笔误：`int i = (int)i;`（本意取 arg 却自赋值，M3D8 真实踩坑 → 打印垃圾值 -1542590405）；应 `long i = (long)arg;`

### 11.2 pthread_self（取自身 tid）
```c
pthread_t pthread_self(void);
```
- 🧪 **亲验要点**：`pthread_t` 在 NPTL 实现里本质是**线程描述符（struct pthread）的内存地址**；打印用 `%lu`（64 位 Linux 可行但非可移植，POSIX 只保证同进程内"可比"）
- **易错点（重要）**：**pthread_t 值会被复用** ——「创建 → 立刻 join 回收 → 再创建下一个」串行模式下，前一线程描述符内存被释放，下一线程分配到**同一地址** → `pthread_self()` 打印值相同（pthread_join_more.c 亲验：5 线程 tid 全部相同，仅 main 不同）。想看 tid 各异：**先全部 create 再统一 join**。故 **pthread_t 不能当线程的永久身份证**（对比 pid 短期内不复用）

### 11.3 pthread_join（阻塞回收 + 取返回值）
```c
int pthread_join(pthread_t thread, void **retval);
```
- **参数**：`thread` 目标线程 tid（同进程、未 detach）；`retval` 出参收线程 `return`/`pthread_exit` 的值（不关心传 `NULL`）
- **返回值**：成功 0；失败错误码（如对已 detach 线程 join → `EINVAL`=22）
- 🧪 **亲验要点**（pthread_join.c + pthread_join_more.c）：
  - 单线程版：线程 `return (void*)123`，主线程 `void *res; pthread_join(tid, (void**)&res); printf("%ld", (long)res);` → 打印 123 ✅
  - 批量版：5 线程各 `return (void*)(i*10)`，循环 create+join → `return code=10/20/30/40/50` 全对 ✅
- **注意点**：join 是**阻塞**精确回收（对比 `waitpid(WNOHANG)` 非阻塞轮询）；"回收 + 取退出状态"双重作用，对标进程的 `waitpid`
- **易错点**：
  - 第二参类型是 `void**`，传 `&res`（`res` 是 `void*`）
  - 对已 detach 线程 join → `EINVAL`（⏳ M3D9 detach_demo 亲验）
  - 多个线程对**同一 tid** join → 未定义行为；join 自己 → 死锁/EDEADLK

### 11.4 pthread_exit（只退自己）
```c
void pthread_exit(void *retval);
```
- **要点**：`retval` 即线程返回值（join 可取）；**只终止调用线程** —— 主线程调 `pthread_exit(NULL)` 后进程继续活到其他线程结束；对比主线程 `return`/`exit()` 会**带走整个进程**（工作线程全被强杀）
- 🧪 **亲验要点**：pthread_join.c / join_more.c 主线程末尾 `pthread_exit(NULL)`，join 完自然退出、无残留 ✅
- **易错点**：`exit()`（进程级，带走全部）vs `pthread_exit()`（线程级，只退自己）——线程函数内误写 `exit(1)` 会杀死全进程

### 11.5 pthread_detach（分离线程）🧪 M3D9 亲验
```c
#include <pthread.h>
int pthread_detach(pthread_t thread);
```
- **参数**：`thread` 已创建的线程 tid（也可在创建时通过 `attr` 设 `detachstate=PTHREAD_CREATE_DETACHED` 实现"出生即分离"）
- **返回值**：成功 **0**；失败返回**错误码**（不设 errno，用 `strerror(ret)` 打错）
- **要点**：标记分离后线程结束**自动回收、无需（也不能）join**（对标进程 `signal(SIGCHLD, SIG_IGN)` 的"内核自动回收"——但进程版拿不到退出状态，这里同理拿不到）
- 🧪 **亲验要点**（pthread_detach.c，`gcc -Wall -Wextra -pthread` 零警告）：创建线程 → `pthread_detach(tid)` 成功 → 紧接着 `pthread_join(tid, NULL)` → 运行打印 `pthread_join error : Invalid argument` ✅，即 **`EINVAL`(22) 铁证**：**join 与 detach 二选一、不可兼得**，对已分离线程再 join 立即失败、不会等待线程结束（输出顺序上 join 报错先于线程自身的 printf 出现，正说明它同步返回、没阻塞等线程）
- **易错点**：
  - ⚠️ **detach 后再 join 必 `EINVAL`**：这是面试常问点，今天已亲手验证
  - 分离线程结束由系统自动回收，没有"僵尸线程"概念（不像进程漏 wait 变 Z），但**也拿不到返回值**——想要返回值就必须 join（二者互斥）
  - 主线程若想"不等待但也不阻塞"，用 detach；想要线程的退出状态，用 join；不要两者都试

### 11.6 M3D8 回填清单
- [x] 11.1 pthread_create：`-pthread` 链接、签名 `void*fn(void*)`、传值、pid 相同 tid 不同（亲验）
- [x] 11.2 pthread_self：pthread_t=描述符地址、串行 join 回收后地址复用 → 5 线程 tid 相同现象（亲验）
- [x] 11.3 pthread_join：阻塞回收 + 取返回值 `(void**)&res`、批量 join 取 `i*10`（亲验）
- [x] 11.4 pthread_exit：只退自己、主线程 return/exit 带走全进程（亲验 + 认知）
- [x] 11.5 pthread_detach：自动回收、detach 后 join 报 EINVAL=22 铁证（M3D9 pthread_detach.c 亲验：零警告、运行打印 `pthread_join error : Invalid argument`）
- [x] 线程同步（互斥锁/条件变量/信号量）→ **第 12 节**（M3D9 完成互斥锁部分；try 锁/死锁/读写锁/条件变量/信号量 留 M3D10）

---

## 12. 线程同步（互斥锁 mutex）⭐ M3D9 新增

> **统一规则**：头文件 `<pthread.h>`；编译必须 `-pthread`；**pthread 系列函数成功返回 0、失败返回错误码且不设置 errno** —— 判 `ret != 0` 后用 `strerror(ret)` 打错，**不能用 perror**。

### 12.1 概念：竞态条件（race condition）与临界区
- **竞态条件**：程序结果**依赖线程执行时序**，换个时序结果就变 → 结果不可复现、不可预测
- **`cnt++` 不是原子操作**：编译后是 **load → add → store 三步**。两个线程都读到 100，各自加 1 都写回 101 → 两次自增只加了一次（**丢失更新 lost update**）
- **临界区**：访问共享资源的那段代码；**同步的目标 = 保证同一时刻只有一个线程进入临界区**
- 🧪 **亲验（race_demo.c）**：5 线程 × 10 万次无锁自增，理论 500000，实测 **208515**（且每次运行都不同）→ 竞态坐实

### 12.2 互斥锁类型与初始化/销毁
```c
#include <pthread.h>
pthread_mutex_t mutex;                                          // 类型声明
int pthread_mutex_init(pthread_mutex_t *mutex,
                       const pthread_mutexattr_t *attr);        // 动态初始化
int pthread_mutex_destroy(pthread_mutex_t *mutex);              // 销毁
```
- **两种初始化**（二选一，别混用）：
  - **静态**：`pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;`（全局/静态变量用，可省掉 init/destroy）
  - **动态**：`pthread_mutex_init(&mutex, NULL)`（`attr=NULL` 用默认属性；可配属性如可重入/健壮锁）
- **返回值**：成功 0 / 错误码（不设 errno）
- **易错点**：
  - 已用 `PTHREAD_MUTEX_INITIALIZER` 静态初始化的锁，**不要再调 init**（重复初始化未定义）
  - 动态初始化的锁用完**必须 destroy**；且 destroy 前不能有线程还持有它
  - 锁**必须先初始化再使用**（未初始化的 `pthread_mutex_t` 内容是随机内存 → 锁行为未定义、可能死锁）

### 12.3 pthread_mutex_lock / unlock（核心）
```c
int pthread_mutex_lock(pthread_mutex_t *mutex);     // 加锁（阻塞）
int pthread_mutex_unlock(pthread_mutex_t *mutex);   // 解锁
```
- **lock 是阻塞的**：锁被别人持有 → 本线程进入内核睡眠等待（futex），**不是忙等**、也不是"跳过"
- **lock/unlock 必须配对**：漏 unlock → 后续所有线程永远阻塞 → **死锁**
- **默认 mutex 不可重入**：同一线程连续 lock 两次 → `EDEADLK`；解锁必须由**加锁的那个线程**来做
- **返回值必须查**：`pthread_mutex_lock` 失败时**不保证锁已被获取**（如 `EDEADLK`/`EOWNERDEAD`）→ 失败分支**不能**去 unlock，否则解锁一把没持有的锁
- 🧪 **亲验（mutex_demo.c）**：lock/unlock 返回值全查、只锁 `cnt++` 一行 → 5 线程 × 10 万次 = **精确 500000**，零警告、连跑稳定

### 12.4 M3D9 实测对照表：并发 × 锁 的 2×2 矩阵（本章精华）
| 线程结构 | 无锁 | 有锁 |
|---|---|---|
| **串行**（create 与 join 同一循环） | 500000 ✅ | 500000 ✅ |
| **并发**（create 一个循环 + join 一个循环） | **208515 ❌**（race_demo.c） | **500000 ✅**（mutex_demo.c） |

- ⚠️ **核心结论：并发才是竞态的根源，互斥锁才是解药**。串行结构下**即使完全不写 lock**，结果也是 500000 —— 也就是说"结果对"并不等于"代码对"
- ⚠️ **面试高频陷阱**：只跑一次看到 500000 就以为没 bug，误判为"这个程序本来就是线程安全的"。**必须构造真并发**（create / join 拆成两个循环）才能暴露问题
- 顺带解释 M3D8 现象：串行 create→join 时，pthread_t 描述符内存被复用 → 5 线程 tid 打印全相同；改成两段循环后线程真正并存，tid 各不相同

### 12.5 锁粒度（性能与正确性的取舍）
- **只锁必要的临界区**：本例只锁 `cnt++` 一行；不要把整个 `for` 循环、`printf` 一起锁进去
- 锁的范围越大 → 阻塞其他线程越久 → **并发度越低、吞吐越低**（加锁后肉眼可见变慢，这是 futex 系统调用的开销）
- 原则：**用性能换正确性**；正确性优先，粒度在保证原子性的前提下尽量小

### 12.6 M3D9 回填清单
- [x] 12.1 竞态条件：`cnt++` 三步非原子、丢失更新（race_demo.c 亲验 208515）
- [x] 12.2 互斥锁初始化：动态 `init`/`destroy`（mutex_demo.c 亲验）；静态 `PTHREAD_MUTEX_INITIALIZER`
- [x] 12.3 lock/unlock：阻塞语义、必须配对、不可重入、返回值必查（用 strerror）
- [x] 12.4 race ↔ mutex 实测对照（2×2 矩阵，含"串行+无锁也 500000"的反直觉结论）
- [x] 12.5 锁粒度：只锁 `cnt++` 一行
- [x] `pthread_mutex_trylock`（非阻塞尝试加锁）→ M3D10 ✅
- [x] 死锁两种成因与解法（同一线程锁两把锁 / 多线程抢多把锁）→ M3D10 ✅
- [x] 读写锁 `pthread_rwlock_*` → M3D10 ✅
- [x] 条件变量 `pthread_cond_*` → M3D10 ✅
- [x] 信号量 `sem_*` → M3D11（见第 15 节）

---

## 13. 线程同步续（trylock / 死锁 / 读写锁）⭐ M3D10 新增

> 统一规则见第 12 节开头：`<pthread.h>` + `-pthread` + 返回错误码用 `strerror(ret)` 打错。

### 13.1 pthread_mutex_trylock（非阻塞尝试加锁）🧪 M3D10 亲验
```c
int pthread_mutex_trylock(pthread_mutex_t *mutex);
```
- **要点**：与 `pthread_mutex_lock` 不同，**不阻塞**——锁空闲则立即获取返回 0；锁被占用则**立即返回 `EBUSY`(16)** 而非等待
- **使用姿势**：`if(ret==0){拿到锁;...;pthread_mutex_unlock;} else if(ret==EBUSY){锁被占，去干别的/稍后重试;} else {其他错误;}`
- **铁律**：**只有真正 trylock 成功的线程才能 unlock**；失败的线程绝不能 unlock（解锁没持有的锁 = 未定义行为）
- 🧪 **亲验（trylock.c）**：main 先 `lock` 持锁 → 子线程 `trylock` 返回 `EBUSY(16)` 打印"锁被占用"；main `unlock` 后再次建子线程 `trylock` 成功 → 一次运行演示"失败+成功"两条路径

### 13.2 死锁（deadlock）🧪 M3D10 亲验
- **四要件（Coffman）**：① 互斥（资源独占）② 占有且等待（持一把等另一把）③ 不可剥夺（锁不能抢）④ **循环等待**（A 等 B、B 等 A）—— 四者同时成立才死锁
- **两类常见场景**：
  - 同一线程对一把**不可重入** mutex 连续 lock 两次 → `EDEADLK` 自锁
  - **多线程抢多把锁、加锁顺序相反**（最典型）：t1 持 A 等 B，t2 持 B 等 A → 循环等待
- **解法①：全局定序法（首选）**：所有线程按**固定顺序**加锁（如一律 A→B）。t1 持 A 时 t2 想拿 A 只能阻塞等 t1 释放，不再各持一把互等 → 死锁消除。🧪 亲验 `deadlock_fix.c`（t2 改成也先锁 A 后锁 B，干净退出）
- **解法②：trylock 回退重试法**：先 lock 第一把 → `trylock` 第二把；失败(`EBUSY`)则**先释放已持有的第一把**、sleep 后重试。🧪 亲验 `deadlock_trylock.c`（打印"get B fail, release lockA, retry"后下一轮成功）。⚠️ **缺点：有活锁(livelock)风险**（两线程同时回退又同时抢），且代码更复杂——一般不如定序法推荐
- 🧪 **复现（deadlock.c）**：t1(A→B)、t2(B→A)，各 `sleep(1)` 确保都拿到第一把锁后再抢第二把 → 稳定卡死，提示符不返回；`kill -9` 退出

### 13.3 读写锁 `pthread_rwlock_t` 🧪 M3D10 亲验
```c
pthread_rwlock_t rw;
pthread_rwlock_init(&rw, NULL);          // 也可用 PTHREAD_RWLOCK_INITIALIZER
pthread_rwlock_rdlock(&rw);              // 读锁：共享（多读者可同时持有）
pthread_rwlock_wrlock(&rw);              // 写锁：独占（与任何读/写互斥）
pthread_rwlock_unlock(&rw);              // 读锁写锁都用它释放
pthread_rwlock_destroy(&rw);
```
- **语义**：**多读可并发、读写互斥、写写互斥**——适合"读多写少"场景（比纯 mutex 并发度高）
- **易错点（血泪）**：`rdlock` 是**共享**的，多个读者**真正并发**执行临界区 → 若在临界区内并发修改某个全局计数（如 `readers_in++`），**该计数本身也会数据竞争**！必须用**另一把 mutex** 单独保护这个计数，且只在"自增/判断/自减"那一下加锁，**别把 sleep 罩进去**（否则读者被这把 mutex 串行化，失去 rwlock 意义）。🧪 亲验 `rwlock.c`：初版 `readers_in++` 裸写在 rdlock 区 → 输出乱序(1/3/2)；加 `cnt_mutex` 后严格 1→2→3，写者看到 0（正确）

### 13.4 M3D10 回填清单（同步续）
- [x] 13.1 trylock：EBUSY=16、成功才 unlock、两路径演示（trylock.c 亲验）
- [x] 13.2 死锁：四要件 / 顺序相反场景 / 定序法 + trylock 回退两解法（deadlock.c / deadlock_fix.c / deadlock_trylock.c 亲验）
- [x] 13.3 读写锁：rdlock 共享、wrlock 独占；rdlock 区内改全局变量仍需另配 mutex（rwlock.c 亲验）
- [x] 信号量 `sem_*` → M3D11（见第 15 节）

## 14. 条件变量（condvar）⭐ M3D10 新增

### 14.1 API
```c
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;   // 也可 pthread_cond_init(&cond, NULL)
int pthread_cond_wait(&cond, &mutex);            // 原子地"释放mutex+阻塞"，唤醒后原子地"重拿mutex"
int pthread_cond_signal(&cond);                  // 唤醒一个等待者
int pthread_cond_broadcast(&cond);               // 唤醒所有等待者
int pthread_cond_destroy(&cond);
```
- **编译**：必须 `-pthread`；**返回错误码不设 errno**，用 `strerror(ret)` 打错

### 14.2 三铁律（错一条就出 bug）
1. **必须配 mutex**：条件变量本身不保护数据，被等待的"共享条件"（如 `ready`）必须由 mutex 保护
2. **`while` 复查，不是 `if`**：`while(条件不满足) pthread_cond_wait(...)`。防**虚假唤醒**(spurious wakeup) 和"多消费者时别人抢先消费"；醒来后无条件再判一次
3. **先改共享状态、再 signal**：生产者在 `ready=1` **之后**才 `pthread_cond_signal` —— 保证消费者被唤醒、`while` 复查时状态已为真，不会漏唤醒

### 14.3 为什么 wait 要传 mutex
`pthread_cond_wait` 内部**原子地释放 mutex 再阻塞**；被唤醒时**原子地重新获取 mutex**。若 wait 不释放锁，生产者永远拿不到 mutex → 死锁。所以调用顺序必须是 `lock → while(!cond) wait → 处理 → unlock`

### 14.4 执行轨迹（单生产者+单消费者）🧪 M3D10 亲验
```
消费者持锁 → ready==0 → 打印"需要等待" → cond_wait(释放锁,阻塞)
生产者 sleep(1) → 持锁 → ready=1 → signal → 解锁
消费者被唤醒 → 重拿锁 → while 复查 ready==1 → 退出循环 → 消费(ready=0) → 解锁
```
🧪 亲验（cond.c，零警告）：输出 `没有产品，消费者1需要等待,ready=0` → `生产者2产品就绪,发送signal信号,ready=1` → `产品就绪，消费者1可以消费,ready=1`，干净退出

### 14.5 易错点
- `if(cond) wait` 误用（应为 `while`）→ 虚假唤醒后带错误状态继续 → 数据错乱
- 先 `signal` 再改状态 → 竞态窗口 → 漏唤醒
- 忘记 `pthread_cond_wait` 要传 mutex → 编译不过或逻辑错
- 多消费者场景用 `signal` 只醒一个够用；多线程混战才考虑 `broadcast`

### 14.6 M3D10 回填清单（condvar）
- [x] 14.1 API：init/wait/signal/broadcast/destroy（cond.c 亲验）
- [x] 14.2 三铁律：配 mutex / while 复查 / 先改状态再 signal
- [x] 14.3 wait 原子释放锁的必要性（不死锁）
- [x] 14.4 生产者-消费者最小版执行轨迹
- [x] 信号量 `sem_*` / 多缓冲生产者消费者 → M3D11（见第 15 节）

---

## 15. 信号量（semaphore）⭐ M3D11 新增

### 15.1 API
```c
#include <semaphore.h>
sem_t sem;
int sem_init(sem_t *sem, int pshared, unsigned int value);  // pshared=0线程间; value=初值
int sem_wait(sem_t *sem);    // P 操作：value>0 则 -1 继续；否则阻塞到有资源
int sem_post(sem_t *sem);    // V 操作：value+1，若有等待者则唤醒一个
int sem_destroy(sem_t *sem);
int sem_getvalue(sem_t *sem, int *sval);  // 读当前值（仅诊断用，非原子）
```
- **编译**：`-pthread`（信号量在 pthread 库里）
- **错误处理与 pthread 相反**：`sem_*` 失败返回 **-1 且置 errno** → 用 `perror` 打；`pthread_*` 返回错误码（不设 errno）→ 用 `strerror(ret)`。混用时别套错（亲验：M3D11 的 sem 全用 `perror`，pthread 走 `err()` 辅助函数，区分正确）

### 15.2 两种用法
- **二值/阻塞唤醒（初值 0）**：`sem_wait` 先阻塞，等对方 `sem_post` 才继续 → 等价"一次性事件通知"。`sem_basic.c` 亲验：消费者先 `sem_wait` 卡住，生产者 `sleep(1)` 后 `sem_post`，消费者才打印"被唤醒"。
- **计数信号量（初值 = N）**：限流 / 资源池。`sem_limit.c` 亲验：初值 3、起 5 个线程，一次只 3 个能进临界区，另 2 个阻塞等 `sem_post` 释放名额（输出"进入"恰好 3 行，离开一个补位一个）。退化为初值 1 即 ≈ 互斥锁（但信号量无"谁持有"概念，post 可由任意线程调，与 mutex 不同）。

### 15.3 三信号量生产者-消费者（环形缓冲）🧪 M3D11 亲验
```c
#define N 4
sem_t empty, full, mutex;   // empty=空闲位(初N)  full=已填位(初0)  mutex=临界区锁(初1)
int buf[N], in=0, out=0;
// 生产者
sem_wait(&empty);  sem_wait(&mutex);   // ⚠ 顺序：先条件信号量，再 mutex；反过来会死锁
buf[in]=item; in=(in+1)%N;
sem_post(&mutex);  sem_post(&full);     // 先放锁再 V（sleep 也放锁外，别持锁睡）
// 消费者
sem_wait(&full);   sem_wait(&mutex);
item=buf[out]; out=(out+1)%N;
sem_post(&mutex);  sem_post(&empty);
```
- **防死锁铁律**：必须先 `sem_wait(empty/full)` 再 `sem_wait(mutex)`。若反过来（先 mutex 再 empty），缓冲满时生产者握锁等 empty、消费者拿不到锁永远 V 不了 → 死锁。口诀：**先等条件信号量，再抢临界区锁**。
- **`in`/`out` 也要 mutex 保护**：多生产者/消费者时，信号量管不了这两个共享下标，得靠 mutex，否则缓冲读写错位；验证用不变量 `empty + full == N`（临界区内快照会因"一个格子在途"出现 N-1，属正常，非 bug）。
- 🧪 亲验（`prod_cons_sem.c`，零警告）：FIFO 配对 `producer in=i item=k` → `consumer out=i item=k`；`in/out` 在 0–3 循环；缓冲填满 4 个再抽干，无死锁无竞态。

### 15.4 信号量 vs 条件变量（同一道题两种写法）
| | condvar | semaphore |
|---|---|---|
| 计数 | 不计数，`signal` 只醒一个，批量场景要自己维护计数 + 循环唤醒 | 自带计数，`post` N 次自动唤醒 N 个 |
| 配套 | 必须另配 mutex + 共享状态变量 | 信号量本身即状态 |
| 多缓冲 PC | 较啰嗦（要自己数 filled 槽） | 三信号量直接建模，代码更短更不易错 |
| 适用 | 一次性"事件通知"、配合 `while` 复查 | 限流、资源池、生产者消费者 |

### 15.5 M3D11 回填清单（semaphore）
- [x] 15.1 API：init/wait/post/destroy/getvalue（sem 用 perror，pthread 用 strerror 区分）
- [x] 15.2 二值阻塞唤醒（sem_basic）+ 计数限流（sem_limit）
- [x] 15.3 三信号量环形缓冲 PC（prod_cons_sem）：先条件后 mutex、in/out 受 mutex 保护
- [x] 15.4 与 condvar 对照
