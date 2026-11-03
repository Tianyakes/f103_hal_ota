# 时钟系统诊断

## ⚠️ 前置要求

**必须先 `connect()` 建立调试会话。** 如果收到 `"no active session"` 错误，需要重新执行 `connect()`。读RCC寄存器前需要先 `halt()` 暂停CPU。

## 适用场景

- 串口波特率不对（乱码）
- 定时器频率不正确
- 外设工作异常但配置看起来正确
- 系统运行速度异常（太快/太慢）
- PLL未锁定

## 核心思路

MCU的所有时钟由时钟树生成。一个错误的时钟源配置会导致**所有**依赖它的外设出问题。

```
HSI/HSE → PLL倍频 → SYSCLK → AHB分频 → APB1/APB2分频
                                              ↓
                                    外设时钟（UART/SPI/TIM...）
```

## ARM Cortex-M：RCC时钟检查

### 第 1 步：读RCC寄存器

以STM32F1为例（从SVD查RCC基地址=0x40021000）：

```
→ halt()
→ memory_read({"address": "0x40021000", "size": 4})   // RCC_CR
→ memory_read({"address": "0x40021004", "size": 4})   // RCC_CFGR
```

### 第 2 步：分析RCC_CR

| 位 | 名称 | 含义 |
|----|------|------|
| [0] | HSION | HSI(8MHz内部RC)开启 |
| [1] | HSIRDY | HSI就绪 |
| [16] | HSEON | HSE(外部晶振)开启 |
| [17] | HSERDY | HSE就绪 |
| [24] | PLLON | PLL开启 |
| [25] | PLLRDY | PLL锁定 |

**常见问题**：
- HSEON=1但HSERDY=0 → 外部晶振未起振（硬件问题：晶振虚焊/负载电容不对）
- PLLON=1但PLLRDY=0 → PLL未锁定（PLL输入源有问题）

### 第 3 步：分析RCC_CFGR

| 位 | 名称 | 含义 |
|----|------|------|
| [1:0] | SW | 系统时钟源选择：00=HSI, 01=HSE, 10=PLL |
| [3:2] | SWS | 系统时钟源状态（实际值） |
| [7:4] | HPRE | AHB预分频 |
| [10:8] | PPRE1 | APB1预分频（低速，最高36MHz） |
| [13:11] | PPRE2 | APB2预分频（高速，最高72MHz） |
| [21:18] | PLLMUL | PLL倍频系数 |
| [16] | PLLSRC | PLL输入源：0=HSI/2, 1=HSE |

**时钟频率计算（STM32F103）**：

```
如果 SW=10 (PLL作为系统时钟):
  如果 PLLSRC=1: PLL输入 = HSE = 8MHz(假设8MHz晶振)
  如果 PLLSRC=0: PLL输入 = HSI/2 = 4MHz
  
  SYSCLK = PLL输入 × (PLLMUL + 2)
  
  PLLMUL=0111(7) → 倍频9倍
  SYSCLK = 8MHz × 9 = 72MHz

AHB时钟 = SYSCLK / HPRE分频
APB1时钟 = AHB / PPRE1分频 (最高36MHz)
APB2时钟 = AHB / PPRE2分频 (最高72MHz)
```

HPRE分频表：

| HPRE值 | 分频 |
|--------|------|
| 0xxx | 不分频 |
| 1000 | /2 |
| 1001 | /4 |
| 1010 | /8 |
| 1011 | /16 |

PPREx分频表：

| PPREx值 | 分频 |
|---------|------|
| 0xx | 不分频 |
| 100 | /2 |
| 101 | /4 |
| 110 | /8 |
| 111 | /16 |

### 第 4 步：验证实际波特率

知道了APB时钟后，可以验证UART波特率：

```
→ memory_read({"address": "0x40013808", "size": 4})   // USART1_BRR
```

```
实际波特率 = APB2时钟 / (16 × BRR)
BRR是定点数：整数部分[15:4]，小数部分[3:0]

例：BRR=0x0271, APB2=72MHz
整数=0x27=39, 小数=0x1=1/16
分频值 = 39 + 1/16 = 39.0625
波特率 = 72000000 / (16 × 39.0625) = 115200
```

## STM32F4时钟系统

与F1类似但PLL结构不同：

```
→ memory_read({"address": "0x40023800", "size": 4})   // RCC_CR
→ memory_read({"address": "0x40023804", "size": 4})   // RCC_PLLCFGR
→ memory_read({"address": "0x40023808", "size": 4})   // RCC_CFGR
```

PLL频率 = (HSE / PLLM) × PLLN / PLLP

## 快速诊断清单

```
→ halt()
→ memory_read({"address": "RCC_CR地址", "size": 4})
→ memory_read({"address": "RCC_CFGR地址", "size": 4})
```

| 检查项 | 如何判断 | 问题 |
|--------|---------|------|
| HSE起振 | HSEON=1且HSERDY=1 | HSERDY=0→晶振问题 |
| PLL锁定 | PLLON=1且PLLRDY=1 | PLLRDY=0→PLL输入异常 |
| 时钟源切换 | SWS==SW | SWS≠SW→切换失败，还在用HSI |
| APB1不超速 | APB1≤36MHz(F1) | 超频导致外设不稳定 |
| 波特率验证 | 计算值=预期值 | 不匹配→时钟配置/BRR配置有误 |
