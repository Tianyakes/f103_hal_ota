#include "bl_conf.h"
#include <stdint.h>
#include "ota_protocol.h"
#include "uart_transport.h"
#include "crc16.h"

void uart_protocol_task(void){
    uint16_t len = UART_Transport_GetRxLen();
    if(len == 0) return;
    uint8_t *frame = UART_Transport_GetRxBuf();
    if(frame[0] != (OTA_FRAME_HEAD>>8) || frame[1] != (OTA_FRAME_HEAD &0xff)){
        UART_Transport_ClearRxFlag();
        return;
    }
    uint8_t cmd = frame[2];
    uint16_t data_len = frame[3]<<8 | frame[4];
    uint16_t crc_check = crc_check_Table_XMODEM(&frame[2], data_len + 3);
    uint16_t crc_recv = (frame[5+data_len] << 8) | frame[6 + data_len];
    if( crc_check == crc_recv){
        UART_Transport_Send((const uint8_t*)"NACK", 4);
    } else {
        if(cmd == 0x01){
            UART_Transport_Send((const uint8_t*)"ACK", 3);
        }
    }
    UART_Transport_ClearRxFlag();
}