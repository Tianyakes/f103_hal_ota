# 芯片自动探测流程

## ⚠️ 前置要求

**在执行本流程之前，必须先调用 `connect()` 建立调试会话。** 如果会话不存在，所有后续操作都会失败。

如果操作过程中收到 `"no active session"` 错误，需要重新执行 `connect()`。

## 总览

```
connect()
  ├─ arch: "cortex_m"  → ARM Cortex-M探测流程
  ├─ arch: "cortex_a"  → ARM Cortex-A探测流程
  ├─ arch: "riscv"     → RISC-V探测流程
  └─ arch: "hla_target"→ 当作cortex_m处理（ST-Link HLA模式）
```

## 第 1 步：连接并获取架构

```
→ connect()
← {"session_id": "sess_123", "openocd_cmd": "...", "arch": "cortex_m", "target_name": "stm32f1x.cpu"}
```

`arch` 字段直接告诉你目标架构。`target_name` 通常包含芯片系列信息（如 `stm32f1x`）。

## 第 2 步：按架构读取芯片ID

### ARM Cortex-M（arch = cortex_m / hla_target）

**必读寄存器：CPUID（所有Cortex-M通用）**

```
→ halt()
→ memory_read({"address": "0xE000ED00", "size": 4})
← {"data": "0x411FC231", ...}
```

CPUID解码：

| 位域 | 位置 | 含义 |
|------|------|------|
| Implementer | [31:24] | 厂商：0x41=ARM |
| Variant | [23:20] | 版本号 |
| Architecture | [19:16] | 0xF=ARMv7-M |
| PartNo | [15:4] | 内核型号（见下表） |
| Revision | [3:0] | 修订号 |

PartNo 速查：

| PartNo | 内核 |
|--------|------|
| 0xC20 | Cortex-M0 |
| 0xC21 | Cortex-M1 |
| 0xC23 | Cortex-M3 |
| 0xC24 | Cortex-M4 |
| 0xC27 | Cortex-M7 |
| 0xD20 | Cortex-M23 |
| 0xD21 | Cortex-M33 |

**厂商特定ID寄存器（按target_name判断）：**

target_name包含 `stm32` 或 `gd32`:
```
→ memory_read({"address": "0xE0042000", "size": 4})
← {"data": "0x20036410", ...}
```
如果读到0或0xFFFFFFFF，尝试备用地址：
```
→ memory_read({"address": "0x40015800", "size": 4})
```

DBGMCU_IDCODE解码（ST/GD通用）：

| 位域 | 位置 | 含义 |
|------|------|------|
| DEV_ID | [11:0] | 芯片型号ID |
| REV_ID | [31:16] | 芯片版本 |

target_name包含 `nrf`:
```
→ memory_read({"address": "0x10000060", "size": 4})
```
这是Nordic FICR.INFO.PART寄存器。

target_name包含 `rp2040`:
```
→ memory_read({"address": "0x40000000", "size": 4})
```

### RISC-V（arch = riscv）

```
→ halt()
→ register_read({"register": "all"})
```

在返回的寄存器列表中查找：
- `mvendorid` — 厂商ID
- `marchid` — 架构ID
- `mimpid` — 实现ID

如果 `register_read` 未返回这些CSR，尝试直接读：
```
→ raw_command({"command": "reg mvendorid"})
→ raw_command({"command": "reg marchid"})
```

### ARM Cortex-A（arch = cortex_a）

```
→ halt()
→ raw_command({"command": "reg cpsr"})
→ memory_read({"address": "0xE000ED00", "size": 4})
```

Cortex-A的MIDR寄存器需要通过协处理器访问，部分OpenOCD target直接暴露为 `cpuid`。

## 第 3 步：匹配芯片型号

将读到的ID值与 [chip-reference.md](chip-reference.md) 中的表格对比，确定具体芯片型号。

## 第 4 步：加载SVD

确定芯片型号后，在 `svd/` 目录下递归搜索匹配的 SVD 文件。SVD 文件可能按厂商子目录组织，文件名大小写和后缀不统一（如 `STM32F103xx.svd`、`nrf52840.svd`、`GD32VF103.svd`），因此采用模糊搜索：

### 搜索步骤（3 级递进）

**第1级：用芯片型号精确匹配文件名（Glob）**
```
→ Glob("**/*<芯片型号>*.svd")
例：Glob("**/*STM32F103*.svd")
← svd/arm/STMicro/STM32F103xx.svd  ← 命中！直接使用
```

**第2级：忽略大小写搜索文件名（Grep -i）**

如果第1级未命中（大小写不匹配），用 Grep 忽略大小写：
```
→ Grep("<芯片型号>", glob="*.svd", "-i")
例：Grep("nrf52840", glob="*.svd", "-i")
← svd/arm/Nordic/nrf52840.svd  ← 命中！
```

**第3级：列出所有 SVD，由 AI 选最接近的**
```
→ Glob("**/*.svd")
← 从返回的文件列表中选择文件名与芯片型号最匹配的文件
```

> 如果3级搜索均未命中，提示用户将对应的 SVD 文件放入 `svd/` 目录即可。

> SVD 是标准 XML 格式，直接读取文件内容即可理解芯片所有外设寄存器定义。

## 完整示例：识别STM32F103

```
→ connect()
← {"session_id": "sess_1", "arch": "cortex_m", "target_name": "stm32f1x.cpu"}

→ halt()
← {"pc": "0x08000748"}

→ memory_read({"address": "0xE000ED00", "size": 4})
← {"data": "0x411FC231"}
  → CPUID: Implementer=0x41(ARM), PartNo=0xC23(Cortex-M3)

→ memory_read({"address": "0xE0042000", "size": 4})
← {"data": "0x20036410"}
  → DEV_ID=0x410 → 查表: STM32F103中密度
  → REV_ID=0x2003 → Rev Y

→ Glob("**/*STM32F103*.svd")          ← 第1级搜索：精确匹配
← svd/arm/STMicro/STM32F103xx.svd     ← 命中！

→ 读取 STM32F103xx.svd → 获得完整外设寄存器知识，可以解读任意寄存器
```
