#ifndef UART_TRANSPORT_H
#define UART_TRANSPORT_H
#include <stdint.h>

#define RX_BUFFER_SIZE 1024  // 缓冲区开大点，能装下最大的 OTA 包

void UART_Transport_Init(void);
void UART_Transport_Send(const uint8_t *data, uint16_t len);
// 供上层查询：当前是否有一包数据准备好了？
uint16_t UART_Transport_GetRxLen(void);
uint8_t* UART_Transport_GetRxBuf(void);
void UART_Transport_ClearRxFlag(void);
#endif