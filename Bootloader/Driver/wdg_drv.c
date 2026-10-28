#include "stm32f1xx_hal.h"

// IWDG_HandleTypeDef hiwdg;

// void WDG_Init(void) {
//     hiwdg.Instance = IWDG;
//     hiwdg.Init.Prescaler = IWDG_PRESCALER_64; // 分频
//     hiwdg.Init.Reload = 4095;                 // 重装载值
//     // 大概超时时间 = (64 * 4095) / 40000 ≈ 6.5 秒
//     HAL_IWDG_Init(&hiwdg);
// }

// void WDG_Feed(void) {
//     HAL_IWDG_Refresh(&hiwdg);
// }