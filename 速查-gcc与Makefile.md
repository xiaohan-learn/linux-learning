---
tags: [linux, 速查, gcc, makefile]
aliases: [gcc速查, Makefile速查, 编译速查]
created: 2026-09-30
---

# 速查 · gcc 与 Makefile

> 配合 [[M2-韦东山应用篇#3 gcc 编译]] 和 [[M2-韦东山应用篇#4 Makefile]] 使用。多源文件编译在 C 仓库 `ch16` 已练过，这里是应用层复习。

## gcc 编译四阶段
```bash
gcc -E hello.c -o hello.i   # 1. 预处理(展开宏/头文件)
gcc -S hello.i -o hello.s   # 2. 编译(生成汇编)
gcc -c hello.s -o hello.o   # 3. 汇编(生成机器码 .o)
gcc hello.o -o hello        # 4. 链接(生成可执行)
# 日常一步到位：
gcc hello.c -o hello
```

## 常用参数
```bash
gcc main.c calc.c -o app          # 多文件一起编
gcc -Wall -Wextra main.c -o app   # 开警告(必开！)
gcc -g main.c -o app              # 带调试信息(gdb 用)
gcc -O2 main.c -o app             # 二级优化
gcc -I./include main.c -o app     # 额外头文件路径
gcc main.c -L./lib -lmylib -o app # 链接库(-L路径 -l库名, 去lib前缀和.a/.so)
gcc -DDEBUG main.c -o app         # 定义宏 DEBUG
```

## 头文件 / 库 概念
- **头文件 `.h`**：声明函数/结构体（告诉编译器"有什么"）
- **源文件 `.c`**：定义实现（"怎么做的"）
- **静态库 `.a`** / **动态库 `.so`**：编译好的可复用代码
- 链接时 `-lxxx` 找 `libxxx.so` / `libxxx.a`

## 多源文件编译（复用 C ch16）
```bash
# 法1：一次列全
gcc main.c calc.c -I. -o app
# 法2：先各编成 .o 再链接(大项目快)
gcc -c main.c && gcc -c calc.c && gcc main.o calc.o -o app
```

## Makefile 速查
```makefile
CC = gcc
CFLAGS = -Wall -g
TARGET = app
OBJS = main.o calc.o

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)      # 注意：命令前是 Tab，不是空格！

main.o: main.c calc.h
	$(CC) $(CFLAGS) -c main.c

calc.o: calc.c calc.h
	$(CC) $(CFLAGS) -c calc.c

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
```
使用：`make` 编译，`make clean` 清理。

### Makefile 要点
- **规则格式**：`目标: 依赖` 换行 `Tab+命令`
- **命令前必须是 Tab**（新手头号坑：用空格会报 `missing separator`）
- **自动变量**：`$@`=目标名，`$^`=所有依赖，`$<`=第一个依赖
- **`.PHONY: clean`**：声明伪目标（不生成同名文件）
- 改了哪个 `.c`，`make` 只重编它（增量编译，省时间）

> [!warning] 新手坑
> - Makefile 命令缩进用 **Tab**，复制粘贴容易变空格 → 报错
> - 忘记 `#include` 对应 `.h` → 隐式声明警告
> - `-Wall` 报的警告别忽略，多半是真 bug
