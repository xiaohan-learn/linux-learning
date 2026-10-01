---
tags: [linux, 速查, 命令]
aliases: [Linux命令速查, 命令行速查]
created: 2026-09-30
updated: 2026-10-01
---

# 速查 · Linux 基础命令

> 配合 [[M2-韦东山应用篇]] 的「Linux 基础命令」章节使用。零基础从 `pwd/ls/cd` 开始练手感。
> 2026-10-01 补全：压缩解压 / 磁盘系统 / WSL 跨系统 / 权限细化 / grep 进阶。

## 路径与目录
```bash
pwd            # 显示当前绝对路径
ls             # 列出当前目录内容
ls -l          # 详细列表（权限/大小/时间）
ls -a          # 含隐藏文件(以.开头)
ls -lh         # 大小用 K/M/G 显示(人类可读)
cd ~           # 回 home
cd ..          # 上级目录
cd -           # 回到上一次目录
mkdir dir      # 建目录
mkdir -p a/b/c # 级联建多层
rmdir dir      # 删空目录
ln -s 目标 链接名 # 软链接(快捷方式)；ln 不加 -s 是硬链接
```

## 文件操作
```bash
cp src dst     # 复制
cp -r dir1 dir2# 递归复制目录
mv old new     # 移动/改名(同一目录=改名,跨目录=移动)
rm file        # 删文件
rm -rf dir     # 递归强制删目录(危险！确认再敲)
touch file     # 新建空文件/更新时间戳
file f         # 看文件类型(文本/二进制/链接...)
du -sh dir     # 看目录占多大(-s 汇总 -h 人类可读)
```

## 查看内容
```bash
cat file       # 查看全部内容(短文件)
more/less file # 分页查看(less 可上下翻,q 退出;more 只能往下)
head -n 5 f    # 看前5行
tail -n 5 f    # 看后5行
tail -f log    # 实时跟踪日志(看程序输出超好用,Ctrl+C 退出)
wc -l file     # 统计行数(-w 词数 -c 字节数)
diff f1 f2     # 对比两个文件差异
```

## 权限（重点）
```bash
# ls -l 输出解读: -rw-r--r-- 
#   首位: -文件 d目录 l链接
#   后9位每3位一组: 属主(u)/属组(g)/其他(o) 各 自的 r w x
# 数字法: r=4 w=2 x=1, 三位数分别是 u/g/o
chmod 755 f    # rwxr-xr-x: 属主全权,组和其他读+执行(目录/可执行程序常用)
chmod 644 f    # rw-r--r--: 属主读写,其他只读(普通文件默认)
chmod +x f     # 三方都加可执行
chmod u+x f    # 只给属主加执行(u/g/o 配 +/-/=/rwx 任意组合)
chmod o-w f    # 去掉其他的写权限
chmod -R 644 d # 递归改整个目录
chown user:grp f # 改属主属组(需 sudo)
sudo cmd       # 以管理员身份执行(输密码不显字符,盲打回车)
```

## 压缩与解压（重点，编译装库天天用）
```bash
tar -czvf pk.tar.gz dir/   # 打包+gzip压缩(c创建 z用gzip v显示过程 f指定文件名)
tar -xzvf pk.tar.gz        # 解压(x解开,其余同上)
tar -tzvf pk.tar.gz        # 只看包里有什么,不解压(t=list)
tar -czvf pk.tar.gz f1 f2  # 打包多个文件
unzip x.zip / zip -r x.zip dir  # zip 格式(Windows 互传常用)
# 记忆: 压=c 解=x 看=t; -z=gzip; 参数顺序习惯 czvf/xzvf
```

## 管道与重定向（神器）
```bash
cmd1 | cmd2         # 把 cmd1 的输出喂给 cmd2
grep "error" log    # 过滤含 error 的行
ps aux | grep java  # 查进程
cmd < file          # 输入重定向: 把文件内容喂给命令(如 wc -l < f)
cmd > file          # 覆盖写(清空再写)
cmd >> file         # 追加写
cmd 2>&1            # 错误输出也合并(排错常用)
cmd > /dev/null     # 丢弃输出(黑洞)
```

## 查找
```bash
find . -name "*.c"      # 当前目录递归找 .c
find / -name "gcc" 2>/dev/null  # 全系统找(忽略错误)
which gcc              # 查命令在哪
grep -rn "main" src/   # -r 递归目录 -n 显示行号
grep -i "error" log    # -i 忽略大小写
grep -v "#" f          # -v 反选: 排除含#的行(看配置文件常用)
grep -E "zip|tar" 列表  # -E 扩展正则(多条件或)
```

## 进程管理
```bash
ps aux          # 看所有进程
top / htop      # 实时资源监控(top 按 q 退出)
kill PID        # 杀进程(发 SIGTERM 请求退出)
kill -9 PID     # 强制杀 SIGKILL(最后的手段;挂起的进程必须-9)
jobs            # 看后台任务
cmd &           # 后台运行
fg              # 拉回前台(fg %1 指定任务号)
bg              # 让挂起的任务在后台继续跑
Ctrl+C          # 终止当前前台程序
Ctrl+Z          # 挂起(冻结!不是退出,配合 bg/fg/kill -9)
```

## 磁盘与系统信息
```bash
df -h           # 看各挂载点磁盘使用率(h 人类可读;关注 Use% 列)
du -sh dir      # 看某目录实际占多大
free -h         # 看内存使用
uname -a        # 看内核版本信息
lsb_release -a  # 看 Ubuntu 版本
history         # 看敲过的命令历史(!编号 可重复执行)
clear / Ctrl+L  # 清屏
```

## 用户与手册
```bash
whoami          # 我是谁(当前用户名)
echo $USER      # 同上(环境变量)
id              # 用户ID和所属组
man ls          # 查 ls 手册(q 退出,/关键词 搜索)
man 2 open      # 查系统调用 open(M2 文件IO 必用;1=命令 2=系统调用 3=库函数)
命令 --help     # 快速看用法(比 man 简洁)
```

## WSL 专区（跨 Windows/WSL）
```bash
/mnt/c          # Windows C 盘挂载点(df -h 里能看到)
/mnt/d          # Windows D 盘
cp /mnt/c/Users/XIAOHAN/Desktop/test.c ~/   # Windows→WSL 拷文件
cp 文件 /mnt/d/xxx/                          # WSL→Windows 存文件
cmd.exe /c dir  # 从 WSL 调 Windows 命令(interop)
explorer.exe .  # 用 Windows 资源管理器打开当前 WSL 目录(超好用)
# 注: cmd.exe 报 "UNC paths are not supported" 警告=无害,照常执行
```

> [!tip] 新手最常忘
> - 删除用 `rm`，**没有回收站**，删了难恢复 → 重要文件先备份
> - 路径区分大小写（`Test.c` ≠ `test.c`）
> - Windows 的 `\` 在 Linux 是 `/`
> - 迷路了 `pwd` 看在哪，`cd ~` 一键回家
> - `cp: cannot stat '...'` = **源路径不存在**，先 `ls` 确认源拼对没
> - 空格 = 参数分隔符：`cd . .` 是两个参数会报错，`cd ..` 才对
> - `Ctrl+Z` 是挂起不是退出；清后台用 `jobs` 查 → `kill -9 %n`
