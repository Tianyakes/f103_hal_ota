#ifndef UART_TRANSPORT_H
#define UART_TRANSPORT_H
#include <stdint.h>
#include "bl_conf.h"

typedef enum{
    RECVI_FAILED = 0,
    RECVI_DONE,
    RECVI_PASSED
}Recvi_Type;

void UART_Transport_Init(void);
void UART_Transport_Send(const uint8_t *data, uint16_t len);
// 供上层查询：当前是否有一包数据准备好了？
uint16_t UART_Transport_GetRxLen(void);
uint8_t* UART_Transport_GetRxBuf(void);
void UART_Transport_ClearRxFlag(void);
Recvi_Type UART_GetRecviFlag(void);
#endif