#include "bl_conf.h"
#include <stdint.h>
#include <stddef.h>
#include "ota_protocol.h"
#include "uart_transport.h"
#include "crc16.h"
#include "flag_manager.h"
#include "bl_log.h"

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

void ota_protocol_ACK(void){
    UART_Transport_Send((const uint8_t*)"ACK", 3);
}

void ota_protocol_NACK(void){
    UART_Transport_Send((const uint8_t*)"NACK", 4);
}

uint8_t ota_protocol_start(OtaFrame_Type *o){
    if(UART_GetRecviFlag() == RECVI_DONE){
        o->len = UART_Transport_GetRxLen();
        if(o->len == 0) return 0;
        o->frame = UART_Transport_GetRxBuf();
        if(o->frame[0] != (OTA_FRAME_HEAD>>8) || o->frame[1] != (OTA_FRAME_HEAD &0xff)){
            return 0;
        }
        o->cmd = o->frame[2];
        o->len = o->frame[3]<<8 | o->frame[4];
        o->index = o->frame[5];
        o->total = o->frame[6];

        uint16_t crc_check = crc_check_Table_XMODEM(o->frame, o->len + 7);
        uint16_t crc_recv = (o->frame[7+o->len] << 8) | o->frame[8 + o->len];
        if( crc_check == crc_recv){
            // UART_Transport_Send((const uint8_t*)"NACK", 4);
            return 1;
        } else {
            return 0;
            // if(o->cmd == 0x01){
            //     UART_Transport_Send((const uint8_t*)"ACK", 3);
            //     // Flag_Set_Updating();
            //     // BootL_LOG_I("Update starting, %d/%d remaining.", index, total);
            // }
        }
        // UART_Transport_ClearRxFlag();
    }
    return 0;
}
