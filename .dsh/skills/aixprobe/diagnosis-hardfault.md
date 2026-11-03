# HardFault / 崩溃诊断

## 适用场景

- 设备HardFault / MemManage / BusFault / UsageFault
- 设备突然卡死无响应
- LED停止闪烁 / 看门狗超时
- 串口日志中断

## 诊断流程

```
connect → halt → 读寄存器 → 判断异常类型
→ 读Fault状态寄存器 → 栈回溯 → 输出报告
```

## 第 0 步：建立连接

**必须先 `connect()` 建立调试会话。** 如果会话不存在，后续所有操作都会返回 `"no active session"` 错误。

```
→ connect()
← {"session_id": "sess_xxx", "arch": "cortex_m", "target_name": "stm32f1x.cpu"}
```

> 如果操作过程中会话断开（收到 `"no active session"` 错误），需要重新执行 `connect()`。

## 第 1 步：获取CPU状态

```
→ halt()
← {pc: "0x08001234"}

→ register_read({"register": "all"})
← {r0: ..., r1: ..., sp: ..., lr: ..., pc: ..., xpsr: ...}
```

## 第 2 步：判断是否在异常上下文

### ARM Cortex-M

**检查 xpsr 低9位（异常号）：**

| 异常号 | 名称 | 常见原因 |
|--------|------|---------|
| 0 | Thread模式 | 正常运行，不在异常中 |
| 3 | HardFault | 非法访问、未对齐、除零、escalation |
| 4 | MemManage | MPU违规、执行不可执行区域 |
| 5 | BusFault | 访问不存在的地址、总线超时 |
| 6 | UsageFault | 未定义指令、非法状态切换、除零 |
| 11 | SVCall | SVC指令触发（一般正常） |
| 14 | PendSV | RTOS上下文切换（一般正常） |
| 15 | SysTick | 系统滴答定时器 |

**检查 lr 值（EXC_RETURN）：**

| lr值 | 含义 |
|------|------|
| 0xFFFFFFF1 | 返回Handler模式，使用MSP（嵌套异常） |
| 0xFFFFFFF9 | 返回Thread模式，使用MSP（无OS场景） |
| 0xFFFFFFFD | 返回Thread模式，使用PSP（RTOS场景） |

### RISC-V

直接读异常寄存器：
```
→ raw_command({"command": "reg mepc"})    // 崩溃点PC
→ raw_command({"command": "reg mcause"})  // 异常原因
→ raw_command({"command": "reg mtval"})   // 异常相关值
```

mcause编码：

| 值 | 原因 |
|----|------|
| 0 | 指令地址未对齐 |
| 1 | 取指访问错误 |
| 2 | 非法指令 |
| 3 | 断点 |
| 4 | Load地址未对齐 |
| 5 | Load访问错误 |
| 6 | Store地址未对齐 |
| 7 | Store访问错误 |

## 第 3 步：读Fault状态寄存器（ARM Cortex-M）

```
→ memory_read({"address": "0xE000ED28", "size": 4})   // CFSR
→ memory_read({"address": "0xE000ED2C", "size": 4})   // HFSR
→ memory_read({"address": "0xE000ED38", "size": 4})   // MMFAR
→ memory_read({"address": "0xE000ED3C", "size": 4})   // BFAR
```

### CFSR位域解析（0xE000ED28）

**MemManage Fault [7:0]：**

| 位 | 名称 | 含义 |
|----|------|------|
| [0] | IACCVIOL | 取指访问违规 |
| [1] | DACCVIOL | 数据访问违规 |
| [3] | MUNSTKERR | 退栈MemManage错误 |
| [4] | MSTKERR | 入栈MemManage错误 |
| [7] | MMARVALID | MMFAR地址有效（=1时读MMFAR获取违规地址） |

**BusFault [15:8]：**

| 位 | 名称 | 含义 |
|----|------|------|
| [8] | IBUSERR | 取指总线错误 |
| [9] | PRECISERR | 精确数据总线错误（BFAR有效） |
| [10] | IMPRECISERR | 非精确总线错误（BFAR可能无效） |
| [11] | UNSTKERR | 退栈总线错误 |
| [12] | STKERR | 入栈总线错误 |
| [15] | BFARVALID | BFAR地址有效 |

**UsageFault [31:16]：**

| 位 | 名称 | 含义 |
|----|------|------|
| [16] | UNDEFINSTR | 未定义指令 |
| [17] | INVSTATE | 无效状态（Thumb位错误） |
| [18] | INVPC | 无效PC加载（EXC_RETURN错误） |
| [19] | NOCP | 协处理器不存在 |
| [24] | UNALIGNED | 非对齐访问 |
| [25] | DIVBYZERO | 除零（需CCR使能DIV_0_TRP） |

### HFSR位域解析（0xE000ED2C）

| 位 | 名称 | 含义 |
|----|------|------|
| [1] | VECTTBL | 向量表读取错误 |
| [30] | FORCED | 强制HardFault（低优先级Fault升级） |
| [31] | DEBUGEVT | 调试事件触发 |

> FORCED=1 时，真正的原因在CFSR中。

## 第 4 步：栈回溯

异常入口时硬件自动压栈，栈帧布局：

```
SP+0x00: R0
SP+0x04: R1
SP+0x08: R2
SP+0x0C: R3
SP+0x10: R12
SP+0x14: LR        ← 异常前的返回地址（谁调用了崩溃函数）
SP+0x18: PC        ← 崩溃点的真正PC（最关键！）
SP+0x1C: xPSR
```

根据lr判断使用MSP还是PSP：
- lr=0xFFFFFFF9 → 用MSP（读 `msp` 寄存器值）
- lr=0xFFFFFFFD → 用PSP（读 `psp` 寄存器值）

```
→ register_read({"register": "msp"})   // 或 psp
← {value: "0x20004FE0"}

→ memory_read({"address": "0x20004FE0", "size": 32})
← {data: "..."}
```

解析：
- 偏移0x18处 = 崩溃时的真正PC
- 偏移0x14处 = 调用者返回地址
- 偏移0x00处 = R0（第一个参数）

## 第 5 步：地址合法性判断

根据connect返回的target_name和chip-reference.md确定合法范围：

| 区域 | 典型范围(STM32F1) | 含义 |
|------|------------------|------|
| Flash | 0x08000000-0x0801FFFF | 代码区 |
| SRAM | 0x20000000-0x20004FFF | 数据区 |
| 外设 | 0x40000000-0x5FFFFFFF | 外设寄存器 |
| Cortex系统 | 0xE0000000-0xE00FFFFF | NVIC/SCB/Debug |

PC不在Flash范围 → 程序跑飞（函数指针错误/栈溢出覆盖返回地址）
SP接近SRAM底部 → 栈溢出风险

## 输出诊断报告模板

```
## HardFault 诊断报告

### 环境
- 芯片: [根据target_name和chip_identify结果]
- 架构: cortex_m

### 异常信息
- 异常类型: [HardFault/BusFault/...]（xpsr异常号=X）
- 当前PC: 0xXXXXXXXX [Flash区/SRAM区/非法]
- 当前SP: 0xXXXXXXXX [栈剩余 XXX 字节]
- LR: 0xXXXXXXXX [EXC_RETURN含义]

### Fault寄存器
- CFSR = 0xXXXXXXXX → [具体错误位]
- HFSR = 0xXXXXXXXX → [FORCED=X]
- BFAR = 0xXXXXXXXX [如果BFARVALID]
- MMFAR = 0xXXXXXXXX [如果MMARVALID]

### 栈回溯
- 崩溃点PC(栈帧): 0xXXXXXXXX
- 调用者LR(栈帧): 0xXXXXXXXX

### 根因分析
[根据CFSR位域+地址范围+栈信息综合判断]

### 常见原因对照
| CFSR标志 | 可能原因 | 检查方向 |
|----------|---------|---------|
| PRECISERR | 访问了不存在的地址 | 检查BFAR指向的地址 |
| INVSTATE | 跳转到ARM模式(非Thumb) | 函数指针最低位未置1 |
| UNDEFINSTR | 执行到数据区 | 栈溢出覆盖返回地址 |
| DIVBYZERO | 除法除数为0 | 检查崩溃点附近的除法 |
| UNALIGNED | 非对齐访问 | 检查指针强转 |

### 建议
1. [具体修复建议]
```
