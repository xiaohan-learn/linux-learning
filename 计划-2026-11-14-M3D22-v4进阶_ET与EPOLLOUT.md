# 计划 · M3D22（2026-11-14 周六）· v4 进阶：ET + EPOLLOUT

> 在已跑通的 v4（`v4_epoll_thread_pool.c`，LT + EPOLLONESHOT）基础上做两件进阶改造：
> 1. **clientfd 从 LT 升级到 ET**（边沿触发，性能更极致，但必须严格非阻塞 + 循环 I/O 到 EAGAIN）；
> 2. **引入 EPOLLOUT 写事件**，解决"慢客户端 / 大响应时 send 阻塞或误杀连接"的隐患——这是生产级服务器的标配。
>
> 产出文件：`linux-tcp-server/v4_et_epollout.c`（**新文件**，保留 `v4_epoll_thread_pool.c` 作为 LT 对照版，便于对比学习）。

---

## 先想清楚：为什么进阶

- 当前 v4 用 **LT**：fd 缓冲有残留数据就一直通知你，好写但不极致；且 worker 里 `send` 若遇对端慢 / 内核发送缓冲满，非阻塞 `send` 返回 `EAGAIN` 后，当前代码直接 `DEL+close` —— **慢客户端会被误杀**（数据没发完连接就断了）。
- **ET（边沿触发）**：只在"不可读→可读"的边沿通知一次，事件次数更少、高并发吞吐更高；但必须"非阻塞 + 循环读/写到 EAGAIN"，否则丢数据。
- **EPOLLOUT（写事件）**：当 `send` 返回 `EAGAIN` 时，不再关连接，而是注册写事件，等内核发送缓冲可写时再续发 —— 需要给每个连接挂一个"待发缓冲区"。

---

## 上午（ET 改造，较轻松）

1. 回顾：讲稿 Q7（LT vs ET）+ `practice/M3D19_epoll/et_echo.c`（你写过的纯 ET demo）。ET 命脉 = **非阻塞 fd + 循环 read 到 EAGAIN**。
2. 改 v4 → `v4_et_epollout.c`：
   - `clientfd` 注册：`EPOLLIN | EPOLLET | EPOLLONESHOT`（加 `EPOLLET`）。
   - `listenfd` **保持 LT 不动**（最安全，避免 accept 漏接；想挑战下午再说）。
   - worker 读路径**基本不用改**——v4 当初就按"非阻塞 recv 循环到 EAGAIN"写的，这正是红利；只需确认 `EPOLL_CTL_MOD` 重新 ARM 时也带 `EPOLLET`（否则下次变回 LT）。
3. 验证（和昨天一样）：编译零警告、多 nc 并发、连发多条全回声、断开无 EBADF。确认 ET 下行为不变。

---

## 下午（EPOLLOUT 写事件，重点 + 难点）

4. 为什么必须 EPOLLOUT：复述上面"慢客户端误杀"问题。
5. 引入**每连接写缓冲**：
   ```c
   #define OUTBUF_MAX 4096
   typedef struct { char buf[OUTBUF_MAX]; int len; int sent; int used; } outbuf_t;
   outbuf_t g_out[65536];   // 用 fd 作下标（demo 够用）
   ```
6. worker 发送逻辑改造成"尝试发 + 发不完挂 EPOLLOUT"：
   - 收到数据 → `send` 循环发；若某次 `send` 返回 `-1 && errno==EAGAIN`：
     - 把**剩余未发数据**拷进 `g_out[fd]`（记 `len`/`sent`），`used=1`；
     - `EPOLL_CTL_MOD` 把该 fd 改成 `EPOLLIN | EPOLLOUT | EPOLLET | EPOLLONESHOT`（继续盯可读，同时盯可写）；
     - worker **返回**（去服务别的 fd），不阻塞等可写。
7. 处理 EPOLLOUT 事件（读路径也要兼容）：worker 接到 fd 后，先判断 `g_out[fd].used`：
   - 有 pending → 循环 `send(g_out[fd].buf + sent, len - sent)` 到 EAGAIN / 发完；
     - 发完：`used=0`，MOD 回 `EPOLLIN | EPOLLET | EPOLLONESHOT`；
     - 还 EAGAIN：保持 `EPOLLOUT` armed（MOD 带 EPOLLOUT），等下次可写再续。
   - 再处理 EPOLLIN（如果这次事件也带了可读）。
8. ET 下注意：`EPOLLOUT` 也是边沿触发，只在"不可写→可写"边沿通知一次 → 续发也必须**循环 send 到 EAGAIN**，发不完才保留 EPOLLOUT。
9. 验证：用一个"慢接收客户端"（如 `nc` 故意不读 / 或用大文件 `cat bigfile | nc ...`）验证：慢客户端**不会被误杀**、数据完整送达、worker 不被 send 阻塞。

---

## 关键新概念（面试能讲）

- **ET 必须配套非阻塞 + 循环 I/O 到 EAGAIN**（读和写都是）。
- **EPOLLOUT 状态机**：send 遇 EAGAIN → 挂写缓冲 + 注册 EPOLLOUT → 等可写续发 → 发完注销 EPOLLOUT。这是 reactor 模型处理"写不完"的标准做法。
- **ONESHOT + 双事件**：一次事件可能同时带 `EPOLLIN|EPOLLOUT`，worker 要分别处理；重新 ARM 时按"是否还有 pending 写"决定带不带 `EPOLLOUT`。

---

## 今日验收

- [ ] 口述 ET 与 LT 在本项目差异、为何 ET 必须循环 I/O
- [ ] `v4_et_epollout.c`（ET 版）编译零警告、多 nc 并发连发 OK
- [ ] 理解并实现 EPOLLOUT 续发 + 每连接写缓冲，慢客户端不误杀、数据完整
- [ ] README / Makefile 加 `v4_et_epollout` 目标（可选，时间够再做）
- [ ] 双仓库（linux-tcp-server / linux_notes）git 双端提交

---

## 完成情况（收工填写）

- [ ] M3D22（11/14）：v4 进阶 ET + EPOLLOUT，编译零警告、慢客户端验证、双仓库提交

---

## 收工 git 命令（WSL 手动执行）

### A. `linux-tcp-server`（进阶代码 + 文档）
```bash
cd /mnt/d/linux-tcp-server
git add v4_et_epollout.c README.md Makefile
git commit -m "feat: v4 进阶 - ET 边沿触发 + EPOLLOUT 写事件（慢客户端不误杀）"
git push github main && git push gitee main
git ls-remote github main && git ls-remote gitee main
```

### B. `linux_notes`（今日计划）
```bash
cd /mnt/d/linux_notes
git add 计划-2026-11-14-M3D22-v4进阶_ET与EPOLLOUT.md
git commit -m "docs: M3D22 v4 进阶 ET+EPOLLOUT 学习计划"
git push github main && git push gitee main
git ls-remote github main && git ls-remote gitee main
```

> 代码写在 `D:\linux-tcp-server\v4_et_epollout.c`。若 EPOLLOUT 下午没写完，先把 ET 部分收工，EPOLLOUT 顺延到 M3D23，别硬凑。
