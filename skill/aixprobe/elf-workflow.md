# ELF符号解析与源码级调试

## ⚠️ 前置要求

**任何需要与目标硬件交互的操作（设断点、读内存、addr2line定位后读寄存器等）都必须先 `connect()` 建立调试会话。** 如果收到 `"no active session"` 错误，需要重新执行 `connect()`。

纯ELF分析操作（readelf、nm、objdump）不需要 connect，但只要涉及硬件读写就需要。

## 适用场景

- 用户提供了编译产物（.elf文件）
- 需要按函数名设断点
- 崩溃时需要定位到源码行
- 需要按变量名读写内存
- 需要查看某函数的反汇编

## 前提

用户PC上需要有交叉编译工具链（安装GCC时自带）：
- ARM: `arm-none-eabi-readelf`, `arm-none-eabi-nm`, `arm-none-eabi-addr2line`, `arm-none-eabi-objdump`
- RISC-V: `riscv64-unknown-elf-readelf`, `riscv64-unknown-elf-nm` 等
- ARM Linux: `aarch64-linux-gnu-readelf` 等

> 如果工具链不在PATH中，用户需要提供完整路径或工具链前缀。

## 第 0 步：检测可用工具链

根据connect返回的arch自动选择前缀，并验证工具可用：

```bash
# ARM Cortex-M
arm-none-eabi-readelf --version 2>/dev/null && echo "arm-none-eabi OK"

# 如果不在PATH，尝试常见安装路径
# Windows: C:/Program Files (x86)/GNU Arm Embedded Toolchain/*/bin/
# Linux: /usr/bin/ 或 /opt/gcc-arm-none-eabi-*/bin/

# RISC-V
riscv64-unknown-elf-readelf --version 2>/dev/null && echo "riscv OK"

# 通用Linux工具也能读大部分ELF（不含反汇编）
readelf --version 2>/dev/null && echo "generic readelf OK"
```

> `readelf`（无前缀）在大多数Linux/WSL上可用，能读任意架构的ELF头和符号表。只有反汇编需要对应架构的objdump。

## 第 1 步：获取ELF基本信息

```bash
# 替换为实际工具链前缀和ELF路径
TOOL_PREFIX="arm-none-eabi-"
ELF="/path/to/firmware.elf"

# ELF头信息（架构、入口点）
${TOOL_PREFIX}readelf -h "$ELF"
```

关键字段：
- `Machine`: 确认架构（ARM, RISC-V等）
- `Entry point address`: 程序入口
- `Type`: EXEC(可执行) 或 DYN(动态)

## 第 2 步：获取段信息（内存布局）

```bash
${TOOL_PREFIX}readelf -S "$ELF"
```

关键段：

| 段名 | 含义 | 调试用途 |
|------|------|---------|
| `.text` | 代码段 | PC应在此范围内 |
| `.rodata` | 只读数据 | 常量字符串 |
| `.data` | 初始化全局变量 | 变量读写 |
| `.bss` | 零初始化全局变量 | 变量读写 |
| `.stack` | 栈区（如有） | 栈溢出检测 |
| `.heap` | 堆区（如有） | 堆使用分析 |

## 第 3 步：提取符号表（核心）

### 获取所有函数符号

```bash
# 列出所有函数，按地址排序
${TOOL_PREFIX}nm -nS "$ELF" | grep " [Tt] "
```

输出格式：`地址 大小 类型 名称`
```
08000100 00000020 T Reset_Handler
08000120 00000004 T NMI_Handler
08000124 00000004 T HardFault_Handler
08001000 00000234 T main
08001234 00000100 T USART1_Init
08001334 00000080 T USART1_Send
...
```

### 获取全局变量

```bash
# 列出所有数据符号（全局/静态变量）
${TOOL_PREFIX}nm -nS "$ELF" | grep " [BbDdGg] "
```

```
20000000 00000004 D SystemCoreClock
20000004 00000001 D uart_rx_flag
20000008 00000100 B rx_buffer
...
```

### 查找特定符号

```bash
# 查找函数
${TOOL_PREFIX}nm "$ELF" | grep -i "main"

# 查找变量
${TOOL_PREFIX}nm "$ELF" | grep -i "uart"
```

## 第 4 步：地址→源码行映射

```bash
# 单个地址
${TOOL_PREFIX}addr2line -e "$ELF" -f -C 0x08001456

# 输出:
# main              ← 函数名
# /src/main.c:87    ← 源文件:行号

# 批量地址（用于栈回溯）
${TOOL_PREFIX}addr2line -e "$ELF" -f -C 0x08001456 0x08001234 0x08001100
```

参数说明：
- `-e`: 指定ELF文件
- `-f`: 同时输出函数名
- `-C`: C++符号反修饰（demangle）

## 第 5 步：反汇编特定函数

```bash
# 反汇编整个文件（大）
${TOOL_PREFIX}objdump -d "$ELF" > disasm.txt

# 只反汇编特定函数（推荐）
${TOOL_PREFIX}objdump -d "$ELF" | sed -n '/<main>:/,/^$/p'

# 反汇编特定地址范围
${TOOL_PREFIX}objdump -d "$ELF" --start-address=0x08001000 --stop-address=0x08001100
```

## 实战场景

### 场景1：按函数名设断点

```
用户: "在main函数入口设断点"

AI执行:
  $ arm-none-eabi-nm firmware.elf | grep " T main$"
  → 08001000 T main

  → breakpoint_set({"address": "0x08001000"})
  ← {success: true}
  → resume()
  // 程序运行到main时断下
```

### 场景2：崩溃地址→源码定位

```
用户: "设备崩溃了，帮我分析"

AI执行:
  → halt()
  → register_read({"register": "all"})
  ← {pc: "0x08002456", xpsr: "0x01000003", sp: "0x20004FE0", lr: "0xFFFFFFFD"}

  // xpsr异常号=3 → HardFault
  // 从栈帧读崩溃点
  → memory_read({"address": "0x20004FE0", "size": 32})
  // 偏移0x18 = 0x080014B2 (真正崩溃PC)
  // 偏移0x14 = 0x08001234 (调用者LR)

  // 翻译为源码位置
  $ arm-none-eabi-addr2line -e firmware.elf -f -C 0x080014B2
  → USART1_Send
  → src/usart.c:42

  $ arm-none-eabi-addr2line -e firmware.elf -f -C 0x08001234
  → main
  → src/main.c:156

  // 查看崩溃点附近反汇编
  $ arm-none-eabi-objdump -d firmware.elf --start-address=0x080014A0 --stop-address=0x080014C0
  → 看到具体指令

  报告:
  "HardFault发生在 src/usart.c:42 (USART1_Send函数)
   由 src/main.c:156 调用"
```

### 场景3：按变量名读内存

```
用户: "读一下SystemCoreClock的值"

AI执行:
  $ arm-none-eabi-nm firmware.elf | grep "SystemCoreClock"
  → 20000000 00000004 D SystemCoreClock

  → memory_read({"address": "0x20000000", "size": 4})
  ← {data: "0x044AA200"}
  // 0x044AA200 = 72000000 = 72MHz

  "SystemCoreClock = 72000000 (72MHz)"
```

### 场景4：确认程序内存布局

```
用户: "帮我看看程序占用了多少Flash和RAM"

AI执行:
  $ arm-none-eabi-readelf -S firmware.elf | grep -E "\.text|\.rodata|\.data|\.bss"
  → .text    08000000  010a0  (代码: 67.6KB)
  → .rodata  080010a0  00500  (只读数据: 1.3KB)
  → .data    20000000  00100  (初始化数据: 256B)
  → .bss     20000100  00800  (BSS: 2KB)

  $ arm-none-eabi-size firmware.elf
  →    text    data     bss     dec     hex filename
  →   70048     256    2048   72352   11aa0 firmware.elf

  "Flash使用: 68.8KB (.text+.rodata+.data初始值)
   RAM使用: 2.3KB (.data+.bss), 栈和堆另计"
```

### 场景5：死循环函数定位

```
PC采样5次都在 0x08003200 附近

AI执行:
  $ arm-none-eabi-addr2line -e firmware.elf -f 0x08003200
  → HAL_Delay
  → Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c:380

  // 原来是卡在HAL_Delay里
  // 检查SysTick是否在运行
  → memory_read({"address": "0xE000E010", "size": 4})  // SysTick_CTRL
  // 如果SysTick未使能，HAL_Delay永远不会返回
```

## 工具链前缀自动检测逻辑

根据connect返回的arch选择：

```
arch="cortex_m" → 尝试顺序:
  1. arm-none-eabi-
  2. arm-linux-gnueabihf- (不常见但可能)
  3. 提示用户指定

arch="riscv" → 尝试顺序:
  1. riscv64-unknown-elf-
  2. riscv32-unknown-elf-
  3. riscv-none-embed-
  4. 提示用户指定

通用后备: readelf / nm / objdump（无前缀，大多数Linux可用）
```

验证命令：
```bash
which ${TOOL_PREFIX}readelf 2>/dev/null || echo "not found"
```

## 注意事项

1. **ELF必须包含符号表**：如果编译时用了 `-s`(strip) 去掉符号，nm/addr2line无法工作。检查方法：
   ```bash
   ${TOOL_PREFIX}readelf -s "$ELF" | head -5
   # 如果输出 "Symbol table '.symtab' contains N entries" 则OK
   # 如果没有.symtab段 → 符号已被strip
   ```

2. **addr2line需要调试信息**：编译时需要 `-g` 选项生成DWARF信息，否则只能得到函数名，没有源码行号。

3. **地址匹配**：ELF中的地址必须与实际烧录到Flash的地址一致。如果用了bootloader做地址偏移，需要注意。

4. **ELF文件路径中有空格**：用引号包裹路径。

5. **Windows环境**：工具链通常安装在 `C:\Program Files (x86)\GNU Arm Embedded Toolchain\` 下，需要加入PATH或使用完整路径。
