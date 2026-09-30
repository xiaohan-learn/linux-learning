---
tags: [linux, 速查, 命令]
aliases: [Linux命令速查, 命令行速查]
created: 2026-09-30
---

# 速查 · Linux 基础命令

> 配合 [[M2-韦东山应用篇]] 的「Linux 基础命令」章节使用。零基础从 `pwd/ls/cd` 开始练手感。

## 路径与目录
```bash
pwd            # 显示当前绝对路径
ls             # 列出当前目录内容
ls -l          # 详细列表（权限/大小/时间）
ls -a          # 含隐藏文件(以.开头)
cd ~           # 回 home
cd ..          # 上级目录
cd -           # 回到上一次目录
mkdir dir      # 建目录
mkdir -p a/b/c # 级联建多层
rmdir dir      # 删空目录
```

## 文件操作
```bash
cp src dst     # 复制
cp -r dir1 dir2# 递归复制目录
mv old new     # 移动/改名
rm file        # 删文件
rm -rf dir     # 递归强制删目录(危险！确认再敲)
touch file     # 新建空文件/更新时间戳
cat file       # 查看全部内容
more/less file # 分页查看(less 可上下翻)
head -n 5 f    # 看前5行
tail -n 5 f    # 看后5行
tail -f log    # 实时跟踪日志(看程序输出超好用)
wc -l file     # 统计行数
```

## 权限（重点）
```bash
ls -l          # 首位 -文件 d目录；后9位 属主/属组/其他 的 rwx
chmod 755 file # rwxr-xr-x：属主可读写执行，其他读执行
chmod +x file  # 加可执行权限(编译出的程序要先 chmod +x 或直接 ./)
chmod -R 644 dir # 递归改
chown user:grp # 改属主(需 sudo)
sudo cmd       # 以管理员身份执行(输密码不显字符，盲打回车)
```

## 管道与重定向（神器）
```bash
cmd1 | cmd2         # 把 cmd1 的输出喂给 cmd2
grep "error" log    # 过滤含 error 的行
ps aux | grep java  # 查进程
> file              # 覆盖写(清空再写)
>> file             # 追加写
2>&1                # 错误输出也合并(排错常用)
```

## 查找
```bash
find . -name "*.c"      # 当前目录递归找 .c
find / -name "gcc" 2>/dev/null  # 全系统找(忽略错误)
which gcc              # 查命令在哪
grep -rn "main" src/   # 在文件内容里搜(类比 rg)
```

## 进程管理
```bash
ps aux          # 看所有进程
top / htop      # 实时资源监控
kill PID        # 杀进程
kill -9 PID     # 强制杀(最后的手段)
jobs            # 看后台任务
cmd &           # 后台运行
fg              # 拉回前台
Ctrl+C          # 终止当前前台程序
Ctrl+Z          # 挂起(配合 bg/fg)
```

> [!tip] 新手最常忘
> - 删除用 `rm`，**没有回收站**，删了难恢复 → 重要文件先备份
> - 路径区分大小写（`Test.c` ≠ `test.c`）
> - Windows 的 `\` 在 Linux 是 `/`
> - 迷路了 `pwd` 看在哪，`cd ~` 一键回家
