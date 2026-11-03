---
name: aixprobe
description: "aixprobe 芯片探测、寄存器解读与调试诊断技能。连接目标设备后自动识别芯片，加载SVD解读寄存器，并提供HardFault崩溃分析、程序挂死/跑飞定位、外设状态检测、时钟系统验证、内存问题排查等调试诊断能力。触发词：'识别芯片', '探测', '寄存器解读', '外设分析', 'HardFault', '崩溃', '死机', '跑飞', '时钟', '栈溢出', 'identify', 'probe', 'decode register', 'peripheral', 'diagnose', 'crash', 'hang'。"
argument-hint: "[scenario] [chip-family]"
---

# AIxProbe - 芯片自动探测与寄存器解读

连接目标MCU后，根据 `connect` 返回的 `arch` 和 `target_name` 自动识别芯片架构，读取芯片ID寄存器确认型号，然后加载本地SVD文件为AI提供完整的外设寄存器知识。

## ⚠️ 前置要求：必须先 connect

**所有 aixprobe 操作（读寄存器、读内存、halt、诊断等）都依赖一个活动的调试会话。**

在执行任何 MCP 工具调用之前，**必须先调用 `connect()`** 建立连接。如果会话已断开或不存在，后续所有操作都会返回 `"no active session"` 错误。

```
❌ 错误流程：register_read("pc") → 报错 "no active session"
✅ 正确流程：connect() → halt() → register_read("pc") → 成功
```

**判断是否需要 connect：**
- 如果上一次操作返回了 `"no active session"` 错误 → 需要重新 `connect()`
- 每次新的对话或长时间未操作后 → 应先 `connect()` 确认连接
- `connect()` 成功后返回 `session_id`、`arch`、`target_name` 等信息，后续操作可直接使用

## 核心流程

```
connect() → 获取arch/target_name/session_id（必须的第一步）
    ↓
halt() → 暂停CPU（读寄存器/内存前通常需要先halt）
    ↓
读取芯片ID寄存器（memory_read）
    ↓
匹配芯片型号 → 加载SVD知识
    ↓
AI可解读任意外设寄存器
```

## 工作流路由

### 芯片探测与寄存器

| 场景 | 参考流程 | 说明 |
|------|---------|------|
| 连接后识别芯片 | [probe-workflow.md](probe-workflow.md) | 自动探测架构+型号 |
| 查看/解读寄存器 | [register-decode.md](register-decode.md) | 用SVD解读寄存器值 |
| 芯片ID速查 | [chip-reference.md](chip-reference.md) | 芯片ID和地址参考表 |
| ELF符号/源码调试 | [elf-workflow.md](elf-workflow.md) | 函数名断点、崩溃→源码行、变量读写 |

### 调试诊断

| 现象 | 诊断流程 | 核心策略 |
|------|---------|---------|
| HardFault/崩溃/异常 | [diagnosis-hardfault.md](diagnosis-hardfault.md) | 读CFSR+栈回溯+Fault地址 |
| 程序挂死/跑飞/不响应 | [diagnosis-hang.md](diagnosis-hang.md) | 多次PC采样+死循环定位 |
| 外设不工作 | [diagnosis-peripheral.md](diagnosis-peripheral.md) | 时钟→GPIO→外设寄存器逐层排查 |
| 时钟/波特率异常 | [diagnosis-clock.md](diagnosis-clock.md) | RCC时钟树分析+频率计算 |
| 栈溢出/内存踩踏 | [diagnosis-memory.md](diagnosis-memory.md) | SP检查+栈canary+内存填充对比 |

## SVD文件查找

本技能自带 SVD 文件（标准 CMSIS-SVD XML 格式），存放在 `svd/` 目录下，可能按厂商子目录组织（如 `svd/arm/STMicro/STM32F103xx.svd`）。

**SVD 文件查找采用递归模糊搜索，不依赖固定路径和文件名：**

确定芯片型号后，使用 Glob/Grep 工具在 `svd/` 目录下递归搜索文件名包含芯片型号的 `.svd` 文件。

### 搜索步骤（3 级递进）

**第1级：精确匹配文件名（Glob）**
```
→ Glob("**/*STM32F103*.svd")
← svd/arm/STMicro/STM32F103xx.svd  ← 命中！直接使用
```

**第2级：忽略大小写搜索文件名（Grep -i）**
```
→ Grep("nrf52840", glob="*.svd", "-i")
← svd/arm/Nordic/nrf52840.svd  ← 命中！
```

**第3级：列出所有 SVD，由 AI 选最接近的**
```
→ Glob("**/*.svd")
← 从列表中选择文件名与芯片型号最匹配的文件
```

> 如果3级搜索均未命中，提示用户将对应的 SVD 文件放入 `svd/` 目录即可。

SVD 文件来源（用户可自行补充到 `svd/` 目录）：
- cmsis-svd-data: https://github.com/cmsis-svd/cmsis-svd-data
- 各芯片厂商 SDK / CMSIS Pack

## 快速开始

用户说"识别一下连接的芯片"、"读RCC寄存器"、"查看PC指针"等任何涉及硬件操作时，按以下步骤操作：

1. **第一步必须是 `connect()`** — 建立调试会话，获取 arch/target_name
2. 如果需要读寄存器/内存，先 `halt()` 暂停CPU
3. 查看connect返回的 `arch` 字段判断架构
4. 按 [probe-workflow.md](probe-workflow.md) 读取芯片ID
5. 用 Glob/Grep 在 `svd/` 目录下模糊搜索对应的 SVD 文件（见"SVD文件查找"章节）
6. 读取SVD中的外设/寄存器定义
7. 用 `memory_read` 读取寄存器，按SVD位域解码

> **重要提醒**：如果任何操作返回 `"no active session"` 错误，立即重新执行 `connect()` 后再继续。
