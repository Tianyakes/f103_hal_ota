// app_check.c
#include "app_check.h"
#include "stm32f1xx_hal.h"

int App_Is_Valid(uint32_t app_addr) {
    // 1. 地址范围检查
    if (app_addr < 0x08000000 || app_addr >= 0x08010000) return -1;
    
    // 2. 取 MSP 和 Reset 向量
    uint32_t msp = *(__IO uint32_t*)app_addr;
    uint32_t reset = *(__IO uint32_t*)(app_addr + 4);
    
    // 3. 检查 MSP 是否在 RAM (0x20000000 ~ 0x20005000)
    if (msp < 0x20000000 || msp > 0x20005000) return -2;
    
    // 4. 检查 Reset 是否在 Flash 范围
    if (reset < 0x08000000 || reset >= 0x08010000) return -3;
    
    return 0; // 合法
}