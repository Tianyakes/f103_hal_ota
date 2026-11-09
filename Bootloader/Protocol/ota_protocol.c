#include "bl_conf.h"
#include <stdint.h>
#include <stddef.h>
#include "ota_protocol.h"
#include "uart_transport.h"
#include "crc16.h"
#include "flag_manager.h"
#include "bl_log.h"


OtaFrame_Type otaFrame;

void OtaFrame_TypeInit(OtaFrame_Type *o){
    o->haed = 0;
    o->cmd = 0;
    o->len = 0;
    o->frame = NULL;
    o->index = 0;
    o->total = 0;
}

void ota_protocol_Clear(void){
    UART_Transport_ClearRxFlag();
}

OtaFrame_Type* ota_protocol_start(void){
    if(UART_GetRecviFlag() == RECVI_DONE){
        otaFrame.len = UART_Transport_GetRxLen();
        if(otaFrame.len == 0) return NULL;
        uint8_t *frame = otaFrame.frame;
        frame = UART_Transport_GetRxBuf();
        if(frame[0] != (OTA_FRAME_HEAD>>8) || frame[1] != (OTA_FRAME_HEAD &0xff)){
            ota_protocol_Clear();
            UART_Transport_Send((const uint8_t*)"NACK", 4);
            return NULL;
        }
        otaFrame.cmd = frame[2];
        otaFrame.len = frame[3]<<8 | frame[4];
        otaFrame.index = frame[5];
        otaFrame.total = frame[6];

        uint16_t crc_check = crc_check_Table_XMODEM(frame, otaFrame.len + 7);
        uint16_t crc_recv = (frame[7+otaFrame.len] << 8) | frame[8 + otaFrame.len];
        if( crc_check == crc_recv){
            // UART_Transport_Send((const uint8_t*)"NACK", 4);
            return &otaFrame;
        } else {
            ota_protocol_Clear();
            UART_Transport_Send((const uint8_t*)"NACK", 4);
            return NULL;
            // if(otaFrame.cmd == 0x01){
            //     UART_Transport_Send((const uint8_t*)"ACK", 3);
            //     // Flag_Set_Updating();
            //     // BootL_LOG_I("Update starting, %d/%d remaining.", index, total);
            // }
        }
        // UART_Transport_ClearRxFlag();
    }
    return NULL;
}
