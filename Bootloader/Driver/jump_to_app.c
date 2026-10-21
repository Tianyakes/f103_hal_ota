#include "bl_conf.h"
#include <stdint.h>
#include "stm32f103xb.h"
#include "cmsis_gcc.h"

#include "bl_log.h"
#include "stm32f1xx_hal.h"

typedef void (*pFunction)(void);

void app_jump(uint32_t app_addr){
    if(app_addr > 0x08000000 && app_addr < 0x08010000){
        uint32_t msp_addr = *(__IO uint32_t*)app_addr;
        if((msp_addr&0x2ffe0000)==0x20000000){
            uint32_t jump_address = *(__IO uint32_t*)(app_addr+4);
            pFunction RstFunction = (pFunction)jump_address;

            __disable_irq();
            SysTick->CTRL = 0;
            SysTick->LOAD = 0;
            SysTick->VAL = 0;

            // HAL库外设反向初始化
            HAL_DeInit();
            // 设置主站指针
            __set_MSP(*(__IO uint32_t*)app_addr);
            // 设置中断向量指令
            SCB->VTOR = app_addr;
            // 跳转函数
            RstFunction();
        }else{
            BootL_LOG_E("Address is not MSP");
        }
    }else{
        BootL_LOG_E("address is not illegal");
    }
}

// #include "stm32f1xx_hal.h"
// #include "bl_conf.h"

// typedef void (*pFunction)(void);

// void jump_to_app(uint32_t app_addr) {
//     uint32_t jump_address;
//     pFunction jump_to_application;

//     // 1. 检查栈顶地址是否合法（RAM 范围）
//     if (((*(__IO uint32_t*)app_addr) & 0x2FFE0000) == 0x20000000) {
//         // 2. 取出复位向量
//         jump_address = *(__IO uint32_t*)(app_addr + 4);
//         jump_to_application = (pFunction)jump_address;

//         // 3. 关掉所有中断和滴答定时器
//         __disable_irq();
//         SysTick->CTRL = 0;
//         SysTick->LOAD = 0;
//         SysTick->VAL  = 0;
        
//         // 4. 反初始化 HAL 库（释放串口等外设）
//         HAL_DeInit();

//         // 5. 设置主堆栈指针（MSP）
//         __set_MSP(*(__IO uint32_t*)app_addr);

//         // 6. 设置中断向量表偏移（VTOR）
//         SCB->VTOR = app_addr;

//         // 7. 跳转
//         jump_to_application();
//     }
// }

