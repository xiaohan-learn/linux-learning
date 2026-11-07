# 计划：M3D16 · System V IPC（消息队列 / 共享内存 / 信号量）

> **日期**：2026-11-07 周六（非周日，正常学习）
> **阶段**：M3 Linux 系统编程 · 进程与 IPC 主线收尾
> **时长**：约 6.5h（6 天制 × 6.5h/天）
> **模式**：自己写完整代码（小 Buddy 只 review/debug，不预给骨架）；收工一次性归仓 + 双端提交

---

## 一、今日目标（一句话）

补齐 M3「进程 / IPC」主线的最后一块——**System V IPC 三件套**（消息队列、共享内存、信号量），并厘清它们与已学 IPC（管道 / mmap / POSIX 信号量 / socket）的本质区别。

---

## 二、M3 主线定位：为什么今天学这个

你已掌握的进程间通信手段：

| 手段 | 学过哪天 | 一句话特征 |
|------|----------|-----------|
| 匿名管道 `pipe` | M3D2 | 血缘进程、字节流、单向 |
| 命名管道 `mkfifo` | M3D3 | 无血缘也可用、字节流、单向 |
| 共享内存 `mmap` | M3D4 | 最快、需配同步、映射文件后端 |
| 信号 `signal/sigaction` | M3D6 | 异步事件通知、非数据通道 |
| socket | M3D12-14 | 跨机/本机、字节流、网络编程 |
| **POSIX 信号量 `sem_*`** | M3D11 | 线程/进程同步原语 |

**System V IPC 是一套独立的内核级 IPC 机制**（诞生早于 POSIX IPC），面试里常拿它和 POSIX IPC 做对比。今天三件套：

1. **消息队列 msg** —— 带类型的消息，可随机读（不像管道只能 FIFO）
2. **共享内存 shm** —— 最快的 IPC，进程直接映射同一块内核内存
3. **信号量 sem** —— 用于进程间同步/互斥（注意：不是用来传数据的）

---

## 三、System V IPC 知识清单

### 3.0 公共机制（三件套通用，先吃透）

- **`key_t` + `ftok`**：System V IPC 用「key」标识一个内核对象，靠 `ftok(path, proj_id)` 从一个**已存在的文件**生成 key（文件必须存在，否则 `ENOENT`）。
- **创建/打开统一套路**：`msgget/shmget/semget` 都吃 `key` 和 `flag`（flag 含 `IPC_CREAT` + 权限位如 `0664`）。key 用 `IPC_PRIVATE` 则总是新建私有对象。
- **`xxxctl(..., IPC_RMID, ...)`**：**删除对象**。System V 对象生命周期是「内核持久」的——进程退出后它还在，不删会一直占着内核资源，必须用 `IPC_RMID` 显式清理。
- **调试命令（必会）**：`ipcs` 查看当前所有消息队列/共享内存/信号量；`ipcrm -q/-m/-s <id>` 手动删。卡住时先 `ipcs` 看是不是有僵尸对象。

### 3.1 消息队列 `msg`

```c
#include <sys/msg.h>
int msgget(key_t key, int msgflg);                       // 创建/打开
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);  // 发送
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg); // 接收
int msgctl(int msqid, int cmd, struct msqid_ds *buf);    // 控制/删除
// 消息结构：第一个字段必须是 long mtype; 其后才是数据
struct msgbuf { long mtype; char mtext[256]; };
```
- `msgtyp` 是接收的核心特性：**`>0` 取类型等于它的第一条；`0` 取任意第一条；`<0` 取类型 ≤ |msgtyp| 中最小的**。
- `msgsz` 只算 `mtext` 长度，**不含 `mtype`**。

### 3.2 共享内存 `shm`

```c
#include <sys/shm.h>
int shmget(key_t key, size_t size, int shmflg);   // 创建/打开（size 要页对齐感，给 4096 之类）
void *shmat(int shmid, const void *shmaddr, int shmflg);  // 映射到本进程地址空间
int shmdt(const void *shmaddr);                   // 解除映射（detach）
int shmctl(int shmid, int cmd, struct shmid_ds *buf);     // 控制/删除
```
- `shmat` 返回映射首地址，当普通指针用；`shmdt` 只是本进程解除，**真正释放要 `shmctl(...,IPC_RMID,...)` 且所有进程都 detach 后才回收**。
- 与 `mmap` 共享内存对比：shm 不落文件、纯内核内存、靠 key 标识；mmap 映射文件后端、靠 fd/open。

### 3.3 信号量 `sem`

```c
#include <sys/sem.h>
int semget(key_t key, int nsems, int semflg);      // 创建/打开（nsems=集合里信号量个数）
int semop(int semid, struct sembuf *sops, size_t nsops);  // P/V 操作
int semctl(int semid, int semnum, int cmd, ...);   // 初始化/删除/取值
// semop 的每次操作：
struct sembuf { unsigned short sem_num; short sem_op; short sem_flg; };
// sem_op:  >0 = V 释放(加);  <0 = P 申请(减,不足则阻塞);  0 = 等待变 0
```
- System V 信号量是「**信号量集合**」(`nsems` 个)，用 `sem_num` 指定操作第几个；POSIX 信号量 `sem_t` 是单个。
- 初始化：只能用 `semctl(semid, 0, SETVAL, val)` 给单个设初值（没有 `sem_init` 那种一行初始化的好事）。
- 与 M3D11 学的 POSIX 信号量对比：sem 集合化、靠 key、内核持久；POSIX 可匿名(线程)或命名(进程)。

---

## 四、时间分配（~6.5h）

| 时段          | 内容                               | 产物        |
| ----------- | -------------------------------- | --------- |
| 上午 1（~1.5h） | 公共机制 + 消息队列：读手册骨架/man、写 msg demo | P1 消息队列收发 |
| 上午 2（~1.5h） | 共享内存 shm：写父子/双进程共享内存 demo        | P2 共享内存   |
| 下午 1（~1.5h） | 信号量 sem：用 sem 保护共享内存的互斥访问        | P3 信号量互斥  |
| 下午 2（~1.5h） | 三件套对比复盘 + `ipcs/ipcrm` 调试实操      | 手册第 18 节  |
| 收工（~0.5h）   | 手册新增章节 + 代码归仓 + 双端提交             | git 收工    |
|             |                                  |           |

> 节奏提示：System V 的 API 比 POSIX 啰嗦（尤其信号量），上午可能偏慢，正常。卡住先把「公共机制」三件套的 key/创建/删除套路背熟，再逐个击破。

---

## 五、动手练习（自己写完整文件，小 Buddy review）

> 不预给骨架。先按上面原型自己写，写完发我（贴代码或说"看沙盒"），我帮你 review/debug。

- **P1 消息队列收发**：父进程 `msgsnd` 发一条 `mtype=1` 的 "hello from parent"；子进程 `msgrcv` 取 `mtype=1` 并打印。验收：子进程正确打印父发的内容；收尾 `msgctl(...,IPC_RMID,...)`。
- **P2 共享内存交换**：用 `shmget`+`shmat` 让父子进程共享一段内存，父写一段字符串、子读出来打印。验收：`ipcs -m` 能看到对象；子读到父写的内容；最后 `shmctl(...,IPC_RMID,...)` + 两边都 `shmdt`。
- **P3 信号量互斥**：在 P2 基础上加一个 System V 信号量（初值 1），父写时 `P`、写完 `V`；子读时同样 `P/V`，模拟互斥访问共享内存。验收：多次运行无数据错乱；`ipcs -s` 能看到信号量并在收尾删除。
- **调试练习**：每个对象创建后跑一次 `ipcs`，确认存在；故意不删再 `ipcs` 看僵尸；用 `ipcrm` 手动清理，体会"内核持久"。

---

## 六、与已学 IPC 的对比认知（面试常考，写进手册）

- **shmget 共享内存 vs mmap 共享内存**：前者纯内核内存不落文件、靠 key 标识、内核持久需 `IPC_RMID`；后者映射文件后端、靠 fd/open、关 fd+munmap 即释放。
- **System V 信号量 vs POSIX 信号量（M3D11）**：前者「信号量集合」、靠 key、内核持久、初始化靠 `semctl SETVAL`；后者 `sem_t` 单个、可匿名(线程)或命名(进程)、`sem_init` 一行初始化。
- **消息队列 vs 管道**：消息队列**带类型、可随机读**(msgtyp)、有边界；管道是字节流、只能 FIFO、无消息边界。
- **共性痛点**：System V 三件套都「内核持久」，忘记 `IPC_RMID` 会泄漏内核对象——这是和管道/mmap 最大的生命周期差异。

---

## 七、高频踩坑预警（写代码时盯紧）

1. `ftok` 的文件**必须已存在**，否则 `ENOENT`；proj_id 取非 0 单字符（如 `'a'`）。
2. 创建 flag 要带权限位：`IPC_CREAT | 0664`，否则别的进程打不开。
3. `msgrcv` 的 `msgsz` 只算数据体、**不含 `mtype`**；消息结构首字段必须是 `long mtype`。
4. `shmat` 返回的指针类型转换；`shmdt` 用原指针；**删对象用 `shmctl IPC_RMID`，且要等全部 detach 才真释放**。
5. 信号量 `sem_op` 正负含义别搞反：负=P 申请、正=V 释放、0=等变 0；`sem_flg` 可加 `IPC_NOWAIT` 防死等。
6. **三件套用完务必 `IPC_RMID` 清理**，并养成 `ipcs` 验证的习惯。

---

## 八、收工动作（WSL 手动执行，小 Buddy 不发自动 push）

代码放 WSL 沙盒 `~/linux-practice/SYSTEMV_IPC/`，收工一次性归仓：

```bash
cd /mnt/d/linux_notes

# 1. 沙盒代码归仓
mkdir -p practice/systemv_ipc
cp ~/linux-practice/SYSTEMV_IPC/*.c practice/systemv_ipc/

# 2. 提交（计划 + 手册新增第18节 + 练习代码）
git add 计划-2026-11-07-M3D16-SystemV_IPC.md 速查-M3进程与IPC函数手册.md practice/systemv_ipc/
git commit -m "M3D16 System V IPC：消息队列/共享内存/信号量 + 手册第18节 + 练习"
git push github main
git push gitee main
git ls-remote github main
git ls-remote gitee main

# 3. 清沙盒
rm -rf ~/linux-practice/SYSTEMV_IPC
```
两 `ls-remote` 哈希一致即收工。（手册第 18 节今日内补：消息队列/共享内存/信号量三小节 + 与已学 IPC 对比表）

---

## 九、明日预告（M3D17 · 周日？→ 查日历）

- 先确认明天是否为周日：2026-11-08 是**周日** → **休息，不排计划**。
- 若需顺延：M3D17（周一 11-09）可进入 **M3 大复盘 / 阶段总结**，把进程、IPC、socket、并发模型串成一张知识网，准备收尾 M3、转 M4（V4L2 摄像头项目）。

> ⚠️ 周日硬约束：明天不学习、不排任务。今天把 System V IPC 收干净即可。
