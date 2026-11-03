# 内存问题诊断

## ⚠️ 前置要求

**必须先 `connect()` 建立调试会话。** 如果收到 `"no active session"` 错误，需要重新执行 `connect()`。读寄存器/内存前需要先 `halt()` 暂停CPU。

## 适用场景

- 栈溢出（Stack Overflow）
- 堆溢出 / malloc失败
- 内存踩踏（变量莫名被改）
- 程序运行一段时间后行为异常
- RTOS任务栈不够

## 第 1 步：栈溢出检测

### 检查当前SP

```
→ halt()
→ register_read({"register": "sp"})
← {value: "0x20000120"}
```

对照芯片内存布局（从SVD或chip-reference.md获取）：

| 芯片 | SRAM起始 | SRAM大小 | 栈通常在高地址 |
|------|---------|---------|---------------|
| STM32F103C8 | 0x20000000 | 20KB | SP初始=0x20005000 |
| STM32F407VG | 0x20000000 | 128KB+64KB | SP初始=0x20020000 |

**判断**：
- SP接近SRAM起始地址（如0x20000100）→ 栈几乎耗尽
- SP低于.bss段末尾 → 栈已溢出到全局变量区

### 检查栈canary（如果启用）

很多RTOS在栈底放magic值（如0xDEADBEEF），栈溢出时会覆盖：

```
// 读取栈底区域（SRAM起始+少量偏移）
→ memory_read({"address": "0x20000000", "size": 64})
// 如果出现大量相同值（0xA5A5A5A5 或 0xDEADBEEF）后突然变为其他数据
// → 溢出边界在此处
```

### 向量表检查

栈溢出严重时会覆盖向量表（如果向量表在SRAM）：

```
// 读VTOR（向量表偏移寄存器）
→ memory_read({"address": "0xE000ED08", "size": 4})
// 如果VTOR指向SRAM，检查向量表内容是否被破坏
```

## 第 2 步：堆状态检查

### FreeRTOS堆

```
// FreeRTOS全局变量（地址从map文件获取）
// xFreeBytesRemaining — 剩余堆空间
// xMinimumEverFreeBytesRemaining — 历史最小剩余
// 用raw_command或memory_read读取

// 如果知道变量地址：
→ memory_read({"address": "变量地址", "size": 4})
```

### 通用malloc堆

堆通常在.bss段之后、栈之前。堆管理器内部数据结构被破坏时：
- malloc返回NULL
- free后程序崩溃（双重释放/野指针释放）

## 第 3 步：内存踩踏定位

### 方法1：断点监控

在怀疑被踩踏的地址设硬件监视断点（如果OpenOCD/芯片支持）：

```
// 使用raw_command设置数据断点（DWT watchpoint）
→ raw_command({"command": "wp 0x20001000 4 w"})
// 当0x20001000被写入时自动断下
→ resume()
// 等待触发...
→ register_read({"register": "pc"})
// PC指向执行写操作的指令
```

### 方法2：内存填充+对比

```
// 在关键区域填充pattern
→ memory_write({"address": "0x20001000", "data": "0xDEADDEAD"})
→ memory_write({"address": "0x20001004", "data": "0xDEADDEAD"})
→ memory_write({"address": "0x20001008", "data": "0xDEADDEAD"})
→ memory_write({"address": "0x2000100C", "data": "0xDEADDEAD"})

→ resume()
// 运行一段时间后
→ halt()

// 重新读取，看哪些位置被修改
→ memory_read({"address": "0x20001000", "size": 16})
```

## 第 4 步：RTOS任务栈检查

### FreeRTOS任务栈水印

FreeRTOS的每个任务栈在创建时会填充0xA5。`uxTaskGetStackHighWaterMark`返回剩余最小值。

通过调试器手动检查：

```
// 读取pxCurrentTCB（当前任务控制块）
// TCB结构中包含pxStack（栈底指针）

// 从栈底向上扫描，找第一个非0xA5A5A5A5的位置
→ memory_read({"address": "栈底地址", "size": 256})
// 连续的0xA5区域大小 = 未使用的栈空间
```

## 内存布局参考

典型嵌入式内存布局（低地址→高地址）：

```
0x20000000 ┌──────────────────┐
           │ .data (初始化全局) │
           ├──────────────────┤
           │ .bss  (零初始化)  │
           ├──────────────────┤
           │ Heap  ↓ 向上增长  │
           │                  │
           │   (空闲区域)      │
           │                  │
           │ Stack ↑ 向下增长  │
0x20005000 └──────────────────┘ ← 初始SP
```

栈和堆相向增长，当它们相遇时 → 栈溢出或堆溢出。

## 输出诊断报告模板

```
## 内存诊断报告

### 内存使用状态
| 项目 | 地址/值 | 状态 |
|------|---------|------|
| SRAM范围 | 0x20000000-0x20004FFF | 20KB |
| 当前SP | 0xXXXX | 距栈底 XXX 字节 |
| 栈使用率 | XX% | [安全/警告/溢出] |
| 堆剩余 | XXX bytes | [如果可读] |

### 诊断结果
[栈溢出/堆耗尽/内存踩踏/正常]

### 建议
1. [增加栈大小/优化递归/检查数组越界]
```
