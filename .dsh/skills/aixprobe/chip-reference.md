# 芯片ID速查表

## ARM Cortex-M 通用

### CPUID @ 0xE000ED00（所有Cortex-M通用）

| CPUID值 | PartNo | 内核 |
|---------|--------|------|
| 0x410CC200 | 0xC20 | Cortex-M0 |
| 0x410CC210 | 0xC21 | Cortex-M1 |
| 0x411FC231 | 0xC23 | Cortex-M3 r1p1 |
| 0x410FC241 | 0xC24 | Cortex-M4 r0p1 |
| 0x411FC271 | 0xC27 | Cortex-M7 r1p1 |
| 0x410FD200 | 0xD20 | Cortex-M23 |
| 0x410FD210 | 0xD21 | Cortex-M33 |

解码方法：`PartNo = (CPUID >> 4) & 0xFFF`

---

## STMicroelectronics（ST）

### DBGMCU_IDCODE 地址
- 主地址：`0xE0042000`
- 备用地址：`0x40015800`（STM32L0/L4/G0/G4/WB/WL等新系列）

### DEV_ID速查（IDCODE低12位）

| DEV_ID | 芯片系列 | 内核 | SVD文件 |
|--------|---------|------|---------|
| 0x410 | STM32F103 中密度 | Cortex-M3 | arm/STM32F103.svd |
| 0x412 | STM32F103 低密度 | Cortex-M3 | arm/STM32F103.svd |
| 0x414 | STM32F103 高密度 | Cortex-M3 | arm/STM32F103.svd |
| 0x413 | STM32F405/407/415/417 | Cortex-M4F | arm/STM32F407.svd |
| 0x419 | STM32F42x/43x | Cortex-M4F | arm/STM32F427.svd |
| 0x421 | STM32F446 | Cortex-M4F | arm/STM32F446.svd |
| 0x431 | STM32F411 | Cortex-M4F | arm/STM32F411.svd |
| 0x441 | STM32F412 | Cortex-M4F | arm/STM32F412.svd |
| 0x449 | STM32F74x/75x | Cortex-M7 | arm/STM32F750.svd |
| 0x451 | STM32F76x/77x | Cortex-M7 | arm/STM32F767.svd |
| 0x450 | STM32H743/745/747/750/753/755/757 | Cortex-M7 | arm/STM32H743.svd |
| 0x416 | STM32L151/152 中密度 | Cortex-M3 | arm/STM32L151.svd |
| 0x429 | STM32L100 | Cortex-M3 | arm/STM32L100.svd |
| 0x435 | STM32L43x/44x | Cortex-M4 | arm/STM32L431.svd |
| 0x460 | STM32G07x/08x | Cortex-M0+ | arm/STM32G071.svd |
| 0x466 | STM32G03x/04x | Cortex-M0+ | arm/STM32G031.svd |
| 0x468 | STM32G43x/44x | Cortex-M4F | arm/STM32G431.svd |
| 0x469 | STM32G47x/48x | Cortex-M4F | arm/STM32G471.svd |

### Flash大小寄存器

| 芯片系列 | 地址 | 单位 |
|---------|------|------|
| STM32F1/F3 | 0x1FFFF7E0 | KB (16bit) |
| STM32F2/F4 | 0x1FFF7A22 | KB (16bit) |
| STM32F7 | 0x1FF0F442 | KB (16bit) |
| STM32H7 | 0x1FF1E880 | KB (16bit) |
| STM32L0/L1 | 0x1FF8004C | KB (16bit) |
| STM32L4 | 0x1FFF75E0 | KB (16bit) |
| STM32G0 | 0x1FFF75E0 | KB (16bit) |
| STM32G4 | 0x1FFF75E0 | KB (16bit) |

读取示例：
```
→ memory_read({"address": "0x1FFFF7E0", "size": 2, "width": 16})
← {"data": "0x0080"}  → 128 (128KB Flash)
```

---

## GigaDevice（兆易创新）

### DBGMCU_IDCODE @ 0xE0042000

| DEV_ID | 芯片系列 | 内核 | SVD文件 |
|--------|---------|------|---------|
| 0x410 | GD32F103 | Cortex-M3 | arm/GD32F103.svd |
| 0x414 | GD32F303 | Cortex-M4 | arm/GD32F303.svd |
| 0x418 | GD32E103 | Cortex-M4 | arm/GD32E103.svd |

> GD32和STM32的DEV_ID有重叠，需要结合CPUID的Variant或读取GD32特有寄存器区分。
> GD32的UID（Unique ID）在 `0x1FFFF7E8`，与STM32格式不同。

---

## WCH（南京沁恒）

### CH32V系列（RISC-V）

通过 `mvendorid` CSR 和 `marchid` CSR 识别：

| mvendorid | marchid | 芯片 | SVD文件 |
|-----------|---------|------|---------|
| 0x00000000 | 0x00000001 | CH32V003 | risc-v/CH32V003.svd |
| 0x00000000 | 0x00000002 | CH32V103 | risc-v/CH32V103.svd |
| 0x00000000 | 0x00000003 | CH32V203 | risc-v/CH32V203.svd |
| 0x00000000 | 0x00000004 | CH32V303/305/307 | risc-v/CH32V303.svd |

### CH32F系列（ARM Cortex-M3）

DBGMCU_IDCODE @ 0xE0042000，解码方式同ST。

---

## Nordic Semiconductor

### FICR寄存器

| 地址 | 内容 |
|------|------|
| 0x10000060 | INFO.PART (芯片型号) |
| 0x10000064 | INFO.VARIANT |
| 0x10000068 | INFO.PACKAGE |
| 0x1000006C | INFO.RAM (KB) |
| 0x10000070 | INFO.FLASH (KB) |

| PART值 | 芯片 | SVD文件 |
|--------|------|---------|
| 0x52832 | nRF52832 | arm/nRF52832.svd |
| 0x52840 | nRF52840 | arm/nRF52840.svd |
| 0x52833 | nRF52833 | arm/nRF52833.svd |

---

## Espressif

### ESP32-C3（RISC-V）

OpenOCD target_name包含 `esp32c3`。芯片ID通过eFuse读取：
```
→ memory_read({"address": "0x60007044", "size": 4})
```

### ESP32（Xtensa）

OpenOCD target_name包含 `esp32`。非标准架构，SVD支持有限。

---

## NXP

### LPC系列

Device ID @ `0x400483F4`:

| 值 | 芯片 |
|----|------|
| 0x26113F37 | LPC1768 |
| 0x26013F37 | LPC1769 |

### Kinetis系列

SIM_SDID @ `0x40048024`

---

## 识别流程决策树

```
connect() → arch?
├── cortex_m / hla_target
│   ├── 读CPUID(0xE000ED00) → 确认内核
│   ├── target_name含"stm32"或"gd32"?
│   │   └── 读DBGMCU_IDCODE(0xE0042000) → 查DEV_ID表
│   ├── target_name含"nrf"?
│   │   └── 读FICR(0x10000060) → 查PART表
│   ├── target_name含"lpc"?
│   │   └── 读DevID(0x400483F4) → 查表
│   └── 未知 → 用CPUID确定内核，提示用户指定SVD
│
├── riscv
│   ├── 读marchid/mvendorid CSR
│   ├── target_name含"ch32"?
│   │   └── 按marchid查CH32V表
│   ├── target_name含"esp32c"?
│   │   └── ESP32-C3
│   └── 未知 → 提示用户指定SVD
│
└── 其他 → 提示用户手动指定芯片和SVD
```
