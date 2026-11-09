#include "bootloader.h"
#include "ota_protocol.h"
#include "bl_log.h"
#include "bl_conf.h"
#include "jump_to_app.h"
#include <stdint.h>

OtaFrame_Type o;
static OTA_Update_t OTA = OTA_IDLE;

void BootLoader_Init(void){
    // 驱动初始化


    // ota接收初始化
    OtaFrame_TypeInit(&o);
}

void BootLoader(void){
    if(OTA == OTA_IDLE){
        app_jump(BOOTL_APPLICATION_ADDR);
    }
    uint8_t ret = ota_protocol_start(&o);
    if(ret){
        if(o.cmd == OTA_CMD_START){
            
        }
        ret = 0;
        ota_protocol_ACK();
    }else {
        ota_protocol_Clear();
        ota_protocol_NACK();
    }
}