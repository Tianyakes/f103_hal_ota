#include "uart_drv.h"
#include "stm32f1xx_hal_uart.h"
// #include <stdint.h>
// #include "bl_conf.h"

void BL_uart_send(const uint8_t *buf, uint16_t len){
    HAL_UART_Transmit(BOOTL_UART_USED, buf, len, 50);
}

// void BootLoader_uart_recv(uint8_t *buf, uint16_t len){
//     HAL_UART_Receive(BOOTL_UART_USED, buf, len, 50);
// }