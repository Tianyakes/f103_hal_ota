# 寄存器解读流程

## 前提

- **必须先 `connect()` 建立调试会话**（如果收到 `"no active session"` 错误，重新执行 `connect()`）
- 已通过 [probe-workflow.md](probe-workflow.md) 识别芯片型号
- 已读取对应SVD文件获得外设寄存器知识

## 解读流程

### 第 1 步：在SVD中查找目标寄存器

SVD XML结构：

```xml
<peripheral>
  <name>RCC</name>
  <baseAddress>0x40021000</baseAddress>
  <registers>
    <register>
      <name>CR</name>
      <addressOffset>0x0</addressOffset>
      <resetValue>0x00000083</resetValue>
      <fields>
        <field>
          <name>HSION</name>
          <bitOffset>0</bitOffset>
          <bitWidth>1</bitWidth>
        </field>
        ...
      </fields>
    </register>
  </registers>
</peripheral>
```

计算绝对地址：`baseAddress + addressOffset` = `0x40021000 + 0x0` = `0x40021000`

### 第 2 步：读取硬件寄存器

```
→ halt()
→ memory_read({"address": "0x40021000", "size": 4})
← {"data": "0x0300FF83", ...}
```

### 第 3 步：按位域解码

从SVD的 `<fields>` 中获取每个位域的名称、偏移、宽度，然后：

```
raw_value = 0x0300FF83

提取公式: field_value = (raw_value >> bit_offset) & ((1 << bit_width) - 1)

HSION  [0]:     (0x0300FF83 >> 0)  & 0x1  = 1  ← HSI振荡器开启
HSIRDY [1]:     (0x0300FF83 >> 1)  & 0x1  = 1  ← HSI就绪
HSEON  [16]:    (0x0300FF83 >> 16) & 0x1  = 1  ← HSE振荡器开启
HSERDY [17]:    (0x0300FF83 >> 17) & 0x1  = 1  ← HSE就绪
PLLON  [24]:    (0x0300FF83 >> 24) & 0x1  = 1  ← PLL开启
PLLRDY [25]:    (0x0300FF83 >> 25) & 0x1  = 1  ← PLL就绪
```

### 第 4 步：输出解读结果

以清晰的表格形式呈现：

```
RCC_CR @ 0x40021000 = 0x0300FF83

| 位域     | 位置    | 值  | 含义 |
|----------|---------|-----|------|
| HSION    | [0]     | 1   | HSI振荡器已开启 |
| HSIRDY   | [1]     | 1   | HSI时钟就绪 |
| HSEON    | [16]    | 1   | HSE振荡器已开启 |
| HSERDY   | [17]    | 1   | HSE时钟就绪 |
| PLLON    | [24]    | 1   | PLL已开启 |
| PLLRDY   | [25]    | 1   | PLL锁定就绪 |
```

## SVD位域格式兼容

SVD中位域有三种表示方式，都要能处理：

**格式1: bitOffset + bitWidth（最常见）**
```xml
<field>
  <name>HSION</name>
  <bitOffset>0</bitOffset>
  <bitWidth>1</bitWidth>
</field>
```

**格式2: lsb + msb**
```xml
<field>
  <name>SW</name>
  <lsb>0</lsb>
  <msb>1</msb>
</field>
```
转换: bitOffset = lsb, bitWidth = msb - lsb + 1

**格式3: bitRange**
```xml
<field>
  <name>SWS</name>
  <bitRange>[3:2]</bitRange>
</field>
```
转换: bitOffset = 2, bitWidth = 3 - 2 + 1 = 2

## derivedFrom处理

SVD中外设可以继承另一个外设的寄存器定义：

```xml
<peripheral derivedFrom="USART1">
  <name>USART2</name>
  <baseAddress>0x40004400</baseAddress>
</peripheral>
```

USART2与USART1有相同的寄存器布局，只是基地址不同。遇到 `derivedFrom` 时，去源外设查找寄存器定义。

## 批量解读示例

用户说"查看RCC所有寄存器"时：

1. 从SVD中列出RCC的所有寄存器及其偏移
2. 对每个寄存器调用 `memory_read` 读取
3. 按位域逐个解码
4. 以表格形式汇总输出

```
→ memory_read({"address": "0x40021000", "size": 4})  // RCC_CR
→ memory_read({"address": "0x40021004", "size": 4})  // RCC_CFGR
→ memory_read({"address": "0x40021008", "size": 4})  // RCC_CIR
...
```

> 可以一次读取多个寄存器来减少请求次数：
> `memory_read({"address": "0x40021000", "size": 32})` 一次读8个32位寄存器。

## 写寄存器

修改特定位域时，需要先读-改-写：

```
1. 读: memory_read({"address": "0x40021000", "size": 4}) → 0x0300FF83
2. 改: 要设置HSEON[16]=0 → new_value = 0x0300FF83 & ~(1<<16) = 0x0200FF83
3. 写: memory_write({"address": "0x40021000", "data": "0x0200FF83"})
4. 验: memory_read({"address": "0x40021000", "size": 4}) → 确认修改生效
```
