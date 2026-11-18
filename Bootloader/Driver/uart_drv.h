#ifndef BOOTL_UART_H
#define BOOTL_UART_H

#define BOOTL_USED_HAL 1
#define BOOTL_USED_STATE 0

#if BOOTL_USED_HAL == 1

#include "bl_conf.h"
extern UART_HandleTypeDef huart1;
#define BOOTL_UART_USED (&huart1)
void BL_uart_send(const uint8_t *buf, uint16_t len);
#endif

#endif