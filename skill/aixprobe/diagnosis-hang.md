# 程序挂死 / 跑飞诊断

## ⚠️ 前置要求

**必须先 `connect()` 建立调试会话。** 如果收到 `"no active session"` 错误，需要重新执行 `connect()`。

## 适用场景

- 设备不响应但没有明确崩溃
- 串口无输出或持续重复某行
- LED停止闪烁
- 程序运行一段时间后某功能不工作
- 看门狗反复复位

## 核心策略：多次采样PC

程序挂死本质是**死循环**或**死锁**。通过多次halt-读PC-resume，看PC分布来判断。

## 第 1 步：PC采样（5次）

```
→ connect()
→ halt()
→ register_read({"register": "pc"})    // pc_1
→ resume()

→ halt()
→ register_read({"register": "pc"})    // pc_2
→ resume()

→ halt()
→ register_read({"register": "pc"})    // pc_3
→ resume()

→ halt()
→ register_read({"register": "pc"})    // pc_4
→ resume()

→ halt()
→ register_read({"register": "pc"})    // pc_5
// 最后一次保持halt，用于后续分析
```

## 第 2 步：分析PC分布

### 情况A：所有PC集中在同一地址附近（±几十字节）

**诊断：死循环**

程序卡在一个循环里，等待某个条件永远不成立。

```
→ register_read({"register": "all"})
```

常见死循环模式：
- `while(FLAG==0)` — 等待中断设标志，但中断未触发
- 忙等外设就绪 — 外设时钟未开或配置错误
- `while(1){}` — 误入空循环（代码逻辑错误）

**进一步分析**：读取死循环附近的内存看指令模式：
```
→ memory_read({"address": "PC附近对齐地址", "size": 32})
```

### 情况B：PC在两个地址间交替

**诊断：两个函数互相调用 或 中断处理循环**

检查xpsr看是否在中断中：
```
→ register_read({"register": "xpsr"})
```
异常号非0说明在中断处理，可能是中断不断触发。

### 情况C：PC每次都不同且分散在Flash范围

**诊断：程序在正常运行**

问题可能不是挂死，而是：
- 特定功能路径未执行到
- 通信协议层问题
- 外设配置错误导致无输出

→ 转到外设诊断流程 [diagnosis-peripheral.md](diagnosis-peripheral.md)

### 情况D：PC不在合法地址范围

**诊断：程序跑飞**

参考chip-reference.md确认Flash/SRAM范围。PC在非法区域说明：
- 栈溢出覆盖了返回地址
- 函数指针被破坏（野指针）
- 中断向量表损坏

检查栈溢出：
```
→ register_read({"register": "sp"})
// SP接近SRAM起始地址 = 栈溢出
```

检查向量表：
```
// 向量表通常在Flash起始
→ memory_read({"address": "0x08000000", "size": 64})
// 偏移0x00 = 初始SP（应指向SRAM末尾）
// 偏移0x04 = Reset_Handler（应在Flash范围）
// 偏移0x0C = HardFault_Handler
```

## 第 3 步：死锁检测（RTOS场景）

如果目标运行FreeRTOS/RT-Thread等RTOS：

```
// 读当前任务TCB（FreeRTOS: pxCurrentTCB是全局变量）
// 地址需要从ELF/map文件获取，或用raw_command查找
→ raw_command({"command": "mdw pxCurrentTCB 1"})
```

死锁表现：多个任务都阻塞在信号量/互斥锁上。

## 第 4 步：检查复位原因

设备反复复位时，读复位状态寄存器：

**STM32 RCC_CSR（0x40021024）：**
```
→ memory_read({"address": "0x40021024", "size": 4})
```

| 位 | 名称 | 含义 |
|----|------|------|
| [26] | LPWRRSTF | 低功耗复位 |
| [27] | WWDGRSTF | 窗口看门狗复位 |
| [28] | IWDGRSTF | 独立看门狗复位 |
| [29] | SFTRSTF | 软件复位 |
| [30] | PORRSTF | 上电复位 |
| [31] | PINRSTF | 引脚复位（NRST） |

> IWDGRSTF=1 → 看门狗超时，说明程序卡死时间超过看门狗周期

## 第 5 步：中断状态检查

如果怀疑中断风暴（中断不断触发）：

```
// NVIC中断活跃寄存器 IABR0-7
→ memory_read({"address": "0xE000E300", "size": 32})

// NVIC中断挂起寄存器 ISPR0-7
→ memory_read({"address": "0xE000E200", "size": 32})

// NVIC中断使能寄存器 ISER0-7
→ memory_read({"address": "0xE000E100", "size": 32})
```

活跃位=1的中断正在执行；挂起位=1的中断等待执行。

## 输出诊断报告模板

```
## 挂死/跑飞诊断报告

### PC采样结果
| 采样 | PC值 | 区域 |
|------|------|------|
| 1 | 0xXXXX | Flash/SRAM/非法 |
| 2 | 0xXXXX | |
| 3 | 0xXXXX | |
| 4 | 0xXXXX | |
| 5 | 0xXXXX | |

### 分布模式
[集中/交替/分散/非法区域]

### 诊断结论
- 类型: [死循环/跑飞/中断风暴/正常运行]
- 位置: 0xXXXXXXXX
- SP: 0xXXXX [栈剩余空间]

### 原因分析
[根据PC分布+寄存器+复位原因综合判断]

### 建议
1. [修复建议]
```
