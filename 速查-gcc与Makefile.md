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

### Makefile 常用函数（应用层高频）
> 语法全是 `$(函数名 参数)`，**参数间用逗号分隔、整体括号里**；函数里再嵌函数就 `$(func1 $(func2 x))`。下面都是写"通用 Makefile"时天天用的。

| 函数                           | 作用                          | 示例 / 效果                                                      |
| ---------------------------- | --------------------------- | ------------------------------------------------------------ |
| `$(wildcard *.c)`            | 展开通配符，返回**当前匹配到的文件列表**      | `SRC = $(wildcard *.c)` → `main.c calc.c`                    |
| `$(patsubst %.c,%.o,$(SRC))` | **模式替换**：把列表中 `.c` 全换成 `.o` | `main.o calc.o`                                              |
| `$(subst from,to,text)`      | 简单文本替换（不按模式，纯字符串）           | `$(subst a,b,abc)` → `bbc`                                   |
| `$(addprefix 前缀,列表)`         | 给列表每项加前缀                    | `$(addprefix obj/, main.o calc.o)` → `obj/main.o obj/calc.o` |
| `$(addsuffix 后缀,列表)`         | 给列表每项加后缀                    | `$(addsuffix .o, main calc)` → `main.o calc.o`               |
| `$(notdir 路径)`               | 去掉目录部分，只留文件名                | `$(notdir src/main.c)` → `main.c`                            |
| `$(dir 路径)`                  | 只留目录部分                      | `$(dir src/main.c)` → `src/`                                 |
| `$(basename 名)`              | 去掉后缀                        | `$(basename main.o)` → `main`                                |
| `$(suffix 名)`                | 只取后缀                        | `$(suffix main.c)` → `.c`                                    |
| `$(sort 列表)`                 | 排序并**去重**                   | `$(sort b a b)` → `a b`                                      |
| `$(filter %.c,列表)`           | 从列表里**筛出**匹配模式的项            | `$(filter %.c, a.c b.h)` → `a.c`                             |
| `$(filter-out %.h,列表)`       | 从列表里**剔除**匹配模式的项            | `$(filter-out %.h, a.c b.h)` → `a.c`                         |
| `$(foreach v,列表,表达式)`        | 遍历列表，每个元素代入 `v` 跑表达式        | 见下例                                                          |
| `$(shell 命令)`                | 调 shell 执行命令，返回输出           | `$(shell ls *.c)` 同 wildcard 但更通用                            |
| `$(strip 文本)`                | 去掉首尾空格、压缩中间多余空格             | `$(strip  a  b )` → `a b`                                    |
| `$(if 条件,真,假)`               | 条件分支；条件非空即"真"               | `$(if $(DEBUG),-g,)`                                         |
| `$(call 变量,参1,参2)`           | 调用"参数化"的变量定义（宏）             | 见下例                                                          |

#### foreach 实例
```makefile
# 把 SRCS 里每个 .c 换成 obj/ 下的 .o
SRCS   = main.c calc.c
OBJS   = $(foreach f,$(SRCS),obj/$(f:.c=.o))   # 结果: obj/main.o obj/calc.o
# 注: $(f:.c=.o) 是 $(patsubst) 的简写形式，等价 $(patsubst %.c,%.o,$(f))
```

#### call 实例（自定义"函数"）
```makefile
# 定义一个带参数的变量（宏）：把源文件映射到目标 .o
mk-obj = $(patsubst %.c,%.o,$(1))      # $(1) 是第一个实参
OBJS   = $(call mk-obj,main.c) $(call mk-obj,calc.c)   # → main.o calc.o
```

#### 一套可复用的"通用 Makefile"骨架（综合上面所有函数）
```makefile
CC      = gcc
CFLAGS  = -Wall -Wextra -g
SRC     = $(wildcard *.c)               # 自动抓当前目录全部 .c
OBJ     = $(patsubst %.c,%.o,$(SRC))    # .c → .o
TARGET  = app

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJ) $(TARGET)
```
> 以后新项目（只要是可执行程序、纯 PC gcc）**直接复制这套**，改 `TARGET` 即可，不用每次手写每个 `.o` 规则。`wildcard` + `patsubst` 把"加文件"从"改 Makefile"变成了"往目录丢 .c"。

> [!warning] 新手坑
> - Makefile 命令缩进用 **Tab**，复制粘贴容易变空格 → 报错
> - 忘记 `#include` 对应 `.h` → 隐式声明警告
> - `-Wall` 报的警告别忽略，多半是真 bug
> - 函数参数用**逗号**分隔，别写空格：`$(patsubst %.c,%.o,x)` ✓，`$(patsubst %.c %.o x)` ✗
