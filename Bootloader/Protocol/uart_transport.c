#include "uart_transport.h"
#include <stdint.h>
#include "stm32f1xx_hal.h"

extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
static uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint16_t rx_len = 0;
static volatile uint16_t rx_ready = 0;

void UART_Transport_Init(void){
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
    HAL_UART_Receive_DMA(&huart1, rx_buffer, RX_BUFFER_SIZE);
}

void UART_Transport_Send(const uint8_t *data, uint16_t len) {
    HAL_UART_Transmit(&huart1, data, len, 100);
}

uint16_t UART_Transport_GetRxLen(void){
    return rx_ready?rx_len:0;
}

uint8_t* UART_Transport_GetRxBuf(void){
    return rx_buffer;
}

// 处理完数据后调用，清空标志并重新启动接收
void UART_Transport_ClearRxFlag(void) {
    rx_ready = 0;
    rx_len = 0;
    
    // 清空 DMA 当前计数器（重新开始计）
    __HAL_DMA_DISABLE(&hdma_usart1_rx);
    hdma_usart1_rx.Instance->CNDTR = RX_BUFFER_SIZE; // 重置缓冲区大小
    __HAL_DMA_ENABLE(&hdma_usart1_rx);
}

void UART_IDLE_Callback(uint16_t len) {
    if (len > 0 && len <= RX_BUFFER_SIZE) {
        rx_len = len;
        rx_ready = 1;  // 告诉上层：一包数据准备好了
    }
}