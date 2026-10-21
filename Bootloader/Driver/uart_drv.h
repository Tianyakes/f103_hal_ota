#ifndef BOOTL_UART_H
#define BOOTL_UART_H

#define BOOTL_USED_HAL 1
#define BOOTL_USED_STATE 0

#if BOOTL_USED_HAL == 1

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_uart.h"
extern UART_HandleTypeDef huart1;
#define BOOTL_UART_USED (&huart1)
void BootLoader_uart_send(const uint8_t *buf, uint16_t len);
#endif

#endif