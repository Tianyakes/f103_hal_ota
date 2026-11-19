#ifndef BOOTL_UART_H
#define BOOTL_UART_H
#include "bl_conf.h"
#if BOOTL_USED_HAL == 1


extern UART_HandleTypeDef huart1;
#define BOOTL_UART_USED (&huart1)
void BL_uart_send(const uint8_t *buf, uint16_t len);

#endif

#endif