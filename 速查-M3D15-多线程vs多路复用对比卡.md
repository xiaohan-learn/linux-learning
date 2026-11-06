# 速查-M3D15-多线程 vs epoll 多路复用 对比卡

> 本文档是对 M3D13（多线程 TCP 服务器）+ M3D14（epoll 多路复用）实战的总结，
> 所有内容都来自亲手跑通的代码与踩过的坑，可直接当面试弹药。
> 对应代码：`practice/M3D13-mt-server/`（mt_echo_server.c、chat_server.c）、`practice/M3D14-epoll/`（epoll_echo.c、epoll_chat.c）。

---

## 1. 两种模型的核心事件驱动方式

### 多线程模型（每连接一线程）
- 内核把每个新连接交给**一个独立线程**去伺候。
- `accept` 一次 → `pthread_create` 一个 worker 线程 → 线程阻塞在自己的 `recv` 上。
- **并发来自「多个执行流并行」**：每个连接一个线程，内核调度这些线程抢占 CPU。
- 共享数据（如在线客户端数组）必须加 `pthread_mutex_t` 保护，否则 race。

### epoll 模型（单线程事件循环）
- 单线程用 `epoll_wait` **同时盯一堆 fd**，谁就绪（可读/可写）内核就通知你。
- `accept` 出的新连接也被 `EPOLL_CTL_ADD` 登记进同一个 epoll 实例。
- **并发来自「一个执行流批量处理多个就绪事件」**：一次 `epoll_wait` 返回 N 个就绪，for 循环逐个处理，全程无线程切换。
- 事件循环在单线程里跑，`g_clients[]` 被**顺序**访问，**天然无锁**。

---

## 2. 六维度对比表（面试高频）

| 维度 | 多线程（每连接一线程） | epoll（单线程事件循环） |
|---|---|---|
| **并发来源** | 多个线程并行执行 | 一个线程批量处理就绪事件 |
| **上下文切换** | 有（线程多时切换开销大，C10K 量级扛不住） | 几乎没有（单线程，只有系统调用切换） |
| **共享状态与锁** | 必须（mutex/rwlock 保护共享数据） | 不需要（单线程顺序访问，无 race） |
| **编程复杂度** | 逻辑直观（每连接一份栈），但锁易出错 | 事件驱动回调式，新手难 Debug，但无锁更省心 |
| **C10K 高并发** | 线程开太多内存/切换爆炸，不擅长 | nginx/redis 同款思路，擅长 |
| **典型故障** | 传参坑（传 `&new_fd` 被覆盖）、死锁、锁粒度 | fd 泄漏（断开没 DEL+close）、busy-loop（EINTR 误写）、误关 listen_fd |

---

## 3. 这两天亲手踩过的坑（写进文档最有说服力）

### 多线程版（M3D13 / M3D14 巩固题）
- ✅ **`malloc` 拷 fd 值传参**：不能传 `&new_fd`（主循环局部变量，下次 accept 会覆盖）；`worker` 里 `int fd=*(int*)arg; free(arg);`。
- 🐞 **`total += n` 放错位置 → 死循环**：放循环外导致 `total` 永远 0，不停重发同一段。
- 🐞 **`for(i=0;i<MAX;i++)` 逗号写成分号** → 广播循环条件失效，消息永远广播不出去。

### epoll 版（M3D14）
- 🐞 **游离的 `close(fd)` 在 else 外** → 误关 listen_fd，服务器只接一个连接就死。
- 🐞 **`send` 循环漏 `total += s`** → 同样死循环（和多线程版同源 bug）。
- 🐞 **客户端断开 `r==0` 用 `break` 而非 `continue`** → 同批其他就绪客户端被丢掉。
- 🐞 **`recv` 出错没 `EPOLL_CTL_DEL + close`** → fd 泄漏，下次还被当成就绪。
- 🐞 **`errno=EINTR` 赋值误写**（少一个 `=`）→ 任何 recv 出错都当 EINTR 重试 → busy-loop + fd 泄漏。
- 🐞 **`printf` 漏 `%d` 参数 / `perror(epoll_wait)` 非字符串** → `-Wall -Wextra` 会报警，别略过。

> 共同点：**「循环发全数据漏 `total += x`」在两种模型里都出现过**，说明这是 TCP 字节流编程的通用易错点，不是某一模型的特例。

---

## 4. 什么时候选哪个（给面试官的回答框架）

- **连接少、业务逻辑复杂、要频繁共享状态** → 多线程更简单直观（每人一份栈，逻辑好写）。
- **连接大、I/O 密集、高并发（C10K+）** → epoll（nginx / redis 都这么干），单线程无锁、无切换开销。
- **现代高并发常是混合方案**：多 worker 进程/线程 + 每线程一个 epoll（如 nginx 的 `worker_processes` + `epoll`），兼得两者优点。
- **关键认知**：epoll **本身不提供并发**，它只是「高效获知哪些 fd 就绪」的机制；真正的并发模型是「怎么用 epoll」——单线程事件循环，还是多线程每线程一个 epoll。

---

## 5. 两份服务器的核心骨架（速查）

### 多线程 echo（M3D13 套路）
```c
// main: accept -> malloc 拷 fd -> pthread_create(worker, p) -> pthread_detach
void *worker(void *arg){
    int fd = *(int*)arg; free(arg);
    char buf[1024];
    while((n=recv(fd,buf,sizeof(buf)-1,0))>0){
        buf[n]='\0';
        int total=0;                       // 循环发全
        while(total<n){ ssize_t m=send(fd,buf+total,n-total,0); total+=m; }
    }
    close(fd); return NULL;
}
```

### epoll echo（M3D14 套路）
```c
// main: epoll_create1 -> EPOLL_CTL_ADD(listen_fd) -> epoll_wait 事件循环
//   listen_fd 就绪 -> accept -> EPOLL_CTL_ADD(newfd)
//   客户端 fd 就绪 -> recv；r>0 回显；r==0 -> EPOLL_CTL_DEL + close
for(int i=0;i<n;i++){
    int fd=events[i].data.fd;
    if(fd==listen_fd){ /* accept + EPOLL_CTL_ADD */ }
    else { /* recv；r==0 时 EPOLL_CTL_DEL+close+continue */ }
}
```

### 广播聊天室对照（最能体现差异）
- **多线程版**：`g_clients[]` 用 `pthread_mutex_t` 包住；`client_add/del/broadcast` 三函数持锁访问。
- **epoll 版**：`g_clients[]` 在单线程事件循环里**顺序**访问，**不加任何锁**也跑通——这就是 epoll 相对多线程最大的一张牌。

---

*生成日期：2026-11-06（M3D15 总结日）*
